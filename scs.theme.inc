<?php
//$Id: scs.theme.inc,v 1.1.2.1.2.10 2010/08/10 22:06:43 coworks Exp $
  
/**
 * @file
 * Select Drupal content to create a newsletter
 */

/**
 * Theme the node selection form
 */
function theme_scs_node_selection($form) {
    // Step 1
    if ($form['step']['#value'] == 1) {
        //Headers
        $headers = array(
      '',
        array(
        'data' => t('Nid'),
        'field' => 'nid',
        'sort' => 'asc',
        ),
        array(
        'data' => t('Title'),
        'field' => 'title',
        ),
        array(
        'data' => t('Section'),
        'field' => 't.tid',
        ),
        array(
        'data' => t('Created'),
        'field' => 'created',
        ),
        );

        $nodes = _scs_get_nodes($headers);
            dpm($nodes);
        $sections = _get_sections($nodes);
        //            dpm($sections);
         
        $data = array();

        $explain = '';

        if (user_access('administer scs')) {
            $explain = '<div class="form-item">' . t('Want to select more nodes? Add more content types ') . l('here', 'admin/settings/simplenews/scs') . '.' . '</div>';
        }
        // this code forces the rows to be grouped by section, regardless of sorting
        if(0) {
        foreach ($sections as $section_nodes) {
            //                dpm($nodes);

            foreach ($section_nodes as $node) {
                if (array_key_exists('nid_' . $node->nid, $form)) {
                    //                        dpm($node);
                    $nodearray = array();
                    $nodearray[] = drupal_render($form['nid_' . $node->nid]);
                    $nodearray[] = $node->nid;
                    $nodearray[] = $node->title;
                    $nodearray[] = $section_nodes['sectiontitle'];
                    $nodearray[] = date('Y-m-d H:i:s',$node->created);                    

                    $data[] = $nodearray;
                }
            }
        }
//        dpm($form);
        return drupal_render($form['newsletter_title']) . drupal_render($form['newsletter_toc']) . drupal_render($form['newsletter_content_type']) . $explain . drupal_render($form['filters']) . theme('table', $headers, $data) . drupal_render($form);
        }
        else 
        {
            foreach ($nodes as $node) {
                if (array_key_exists('nid_' . $node->nid, $form)) {
                    //                        dpm($node);
                    $nodearray = array();
                    $nodearray[] = drupal_render($form['nid_' . $node->nid]);
                    $nodearray[] = $node->nid;
                    $nodearray[] = $node->title;
                    $nodearray[] = $node->section;
                    $nodearray[] = $node->created;                    

                    $data[] = $nodearray;
                }
            }
//        dpm($form);
        return drupal_render($form['newsletter_title']) . drupal_render($form['newsletter_toc']) . drupal_render($form['newsletter_content_type']) . $explain . drupal_render($form['filters']) . theme('table', $headers, $data) . drupal_render($form);
        }
    }
    // Step 2
      else {
    return theme('scs_sort_nodes', $form);
  }
}
  
/**
 * Each selected node goes true this function to create a nice body
 * -- overriding this to remove the css id attribute which doesn't validate for
 * a page with multiple sections and multiple tables of contents
 */
function theme_scs_node_titles($titles) {
    if (variable_get('scs_format', 'plain') == 'plain') {
        return implode("\n", $titles);
    }
    else {
        return '<div>' . theme('item_list', $titles) . '</div>';
    }
}

/**
 * Each selected node goes through this function to create a nice body
 */
function theme_scs_node_output($node) {
    $output = '';

    $output = '<div id="node_' . $node->nid . '" class="scs_node">';
    $output .= '<h1 class="node-title">' . l(t($node->title), 'node/' . $node->nid) . '</h1>';
    $output .= '<p>' . node_teaser($node->body) . '</p>';
    $output .= '<p>' . l(t('Read more'), 'node/' . $node->nid) . '</p>';
    $output .= '</div>';

    return $output;
}
  
/**
 * Theme the node sort form into a table
 */
function theme_scs_sort_nodes($form) {
    $headers = array(t('Node title'), t('Section'), t('Weight'));
    $rows = array();
    $nodes = array();
    $node_sections = array();
    foreach ($form as $name=>$field) {
        if (ereg('weight_', $name)) {
            $nid = explode('_', $name);
            $nid = $nid[1];
            $nodes[$nid] = node_load($nid);
            $form[$name]['#taxonomy'] = $nodes[$nid]->taxonomy;
            if(is_array($nodes[$nid]->taxonomy)) {
                $section_names = array();
                foreach($nodes[$nid]->taxonomy as $ktid => $taxon) {
                    $section_names[] = $taxon->name;
                }
                $node_sections[$name] = implode(', ', $section_names);
            }
        }
    }
//    dpm ($form);
    foreach ($form as $name=>$field) {
        if (ereg('weight_', $name)) {
            $nid = explode('_', $name);
            $nid = $nid[1];
            $title = db_result(db_query('SELECT title FROM {node} WHERE nid = %d', $nid));
            unset($form[$name]['#title']);
            
            $row = array();
            $row[] = $title;
            $row[] = $node_sections[$name];
            $row[] = drupal_render($form[$name]);
            $rows[] = array(
        'data' => $row,
        'class' => 'draggable',
            );

        }
    } 
// [TODO] What we really want to do is divide the form by section
// with dragging isolated to each section
//    $sections = _get_sections($nodes);
//    foreach($sections as $section) {
//
//    }
    
    return drupal_render($form['newsletter_title']) . drupal_render($form['newsletter_toc']) . drupal_render($form['newsletter_content_type']) . theme('table', $headers, $rows, array('id' => 'scs_node_sort_table')) . drupal_render($form);
}
      
/**
 * Theme a complete newsletter.
 * group articles into sections in the newsletter by taxonomy term
 * if an article has more than on taxonomy term, it will appear in
 * multiple sections!
 * if "table of contents" is selected, a list of links to each article
 * will appear at the top of each section
 *
 * @param $nodes
 * @param $toc
 */
function theme_scs_newsletter_output($nodes, $toc) {

    $body = '';
    // Node information
    $sections = array();

    // create an array of sections (by tid), each with a subarray of corresponding nodes
    $sections = _get_sections($nodes);

    // order the sections by the weights of their corresponding taxonomy terms
    uasort($sections, 'scs_newsletter_section_sort');

    // render each section
    foreach($sections as $stid =>$nodes) {
        $titles = array();
        $sectionbody = '';
        // create wrapper for section and add section title
        $body .= '<div class="scs_newsletter_section" id="scs_newsletter_section_id_'.$stid.'">';
        $body .= '<h1 class="scs_newsletter_section_title">';
        $body .= $nodes['sectiontitle'];
        $body .= '</h1>';
        // render the toc and nodes for each section
        foreach ($nodes as $node) {
            if($node->nid){
                // ToC (if required)
                if ($toc) {
                    if (variable_get('scs_format', 'plain') == 'plain') {
                        $titles[] = $node->title;
                    }
                    else {
                        $titles[] = '<a href="#node_' . $node->nid . '">' . $node->title . '</a>';
                    }
                }
                $sectionbody .= theme('scs_node_output', $node);

            }
        }
        if ($toc) {
            $body .= theme('scs_node_titles', $titles);
        }

        $body .= $sectionbody.'</div>';
    }
    // Complete newsletter body
    return $body;
    }