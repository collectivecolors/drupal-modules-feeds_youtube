<?php
// $Id$

/**
 * @file
 * Home of the FeedsYouTubeVideoSearchFetcher.
 */

/**
 * Fetches YouTube videos based on sepecified search criteria.
 *   
 * This class basically exists to make creating feed nodes that fetch YouTube 
 * videos easier for the end user.
 */
class FeedsYouTubeVideoSearchFetcher extends FeedsHTTPFetcher {
  
  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    $defaults = parent::configDefaults();
    unset($defaults['auto_detect_feeds']);
    
    $defaults['search_url'] = 'http://gdata.youtube.com/feeds/api/videos';
    $defaults['format']     = 'atom';
       
    return $defaults;
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = parent::configForm($form_state);
    unset($form['auto_detect_feeds']);
    
    $form['search_url'] = array(
      '#type' => 'textfield',
      '#title' => t('YouTube feed endpoint'),
      '#description' => t('If you change this URL be sure that the same query parameters are used.  If you empty this input then submit the default URL will be used'),
      '#default_value' => $this->config['search_url'],
    );    
    $form['format'] = array(
      '#type' => 'select',
      '#title' => t('Format'),
      '#options' => array(
        'atom' => t('Atom'),
        'rss' => t('RSS'),
        'json' => t('JSON'),
      ),
      '#default_value' => $this->config['format'],
    );
    
    return $form;
  }
  
  /**
   * Override parent::configFormValidate().
   */
  public function configFormValidate(&$values) {
    parent::configFormValidate($values);
    
    if (!$values['search_url']) { // TODO: This should really be a check for a valid URL.
      $defaults = $this->configDefaults();
      $values['search_url'] = $defaults['search_url'];
    }
  }
  
  /**
   * Expose source form.
   */
  public function sourceForm($source_config) {
    ctools_include('dependent');
    
    $form['query'] = array(
      '#type' => 'textfield',
      '#title' => t('Optional search query'),
      '#description' => t('This query searches all video titles, keywords, descriptions, author usernames, and categories.  You can also use [ - (NOT) ] to negate a term or [ | (OR) ] to conditionally match terms.'),
      '#default_value' => ($source_config['query'] ? $source_config['query'] : ''),
    );
    
    $form['authors'] = array(
      '#type' => 'textfield',
      '#title' => t('Optional author names'),
      '#description' => t('Enter one of more YouTube user names separated by a comma.'),
      '#default_value' => ($source_config['authors'] ? $source_config['authors'] : ''),
    );
    $form['tags'] = array(
      '#type' => 'textfield',
      '#title' => t('Optional video tags'),
      '#description' => t('Enter one of more YouTube video tags separated by a comma.'),
      '#default_value' => ($source_config['tags'] ? $source_config['tags'] : ''),
    );
    
    $form['use_location'] = array(
      '#type' => 'checkbox',
      '#id' => 'youtube-search-location',
      '#title' => t('Limit posts to a specific geographic area'),
      '#default_value' => ($source_config['use_location'] ? $source_config['use_location'] : 0),
    );
    $form['location'] = array(
      '#type' => 'textfield',
      '#title' => t('Geo-Coordinates for search query'),
      '#description' => t('Enter a latitude and longitude separated by a comma.'),
      '#default_value' => ($source_config['location'] ? $source_config['location'] : ''),
      //'#size' => 15,
      //'#maxlength' => 15,
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('youtube-search-location' => array(1)),
    );
    $form['distance'] = array(
      '#type' => 'textfield',
      '#title' => t('Distance from coordinates'),
      //'#description' => t('Distance (radius) from coordinates to include.'),
      '#default_value' => ($source_config['distance'] ? $source_config['distance'] : 1),
      '#size' => 15,
      '#maxlength' => 15,
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('youtube-search-location' => array(1)),
      '#prefix' => '<div style="float: left;">',
      '#suffix' => '</div>',
    );    
    $form['distance_unit'] = array(
      '#type' => 'select',
      '#title' => t('Unit'),
      '#options' => array(
        'm' => t('Meters'),
        'km' => t('Kilometers'),
        'ft' => t('Feet'),
        'mi' => t('Miles'),
      ),
      '#default_value' => ($source_config['distance_unit'] ? $source_config['distance_unit'] : 'mi'),
      '#process' => array('ctools_dependent_process'),
      '#dependency' => array('youtube-search-location' => array(1)),
      '#prefix' => '<div style="float: left;">',
      '#suffix' => '</div>',
    );
    
    $form['safe_search'] = array(
      '#type' => 'select',
      '#title' => t('Safe search settings'),
      '#options' => array(
        'none' => t('No filtering of videos'),
        'moderate' => t('Some filtering with no restricted content in locale'),
        'strict' => t('Heavy filtering of videos'),
      ),
      '#default_value' => ($source_config['safe_search'] ? $source_config['safe_search'] : 'moderate'),
      '#prefix' => '<div style="clear: both;"></div>',
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    
    if ($values['use_location']) {
      list($latitude, $longitude) = explode(',', $values['location']);
      
      if (!is_numeric($latitude) || !is_numeric($longitude)) {
        form_set_error('location', t('Location must be in the format; latitude, longitude.'));
        $error = TRUE;  
      }
      if (!is_numeric($values['distance']) || $values['distance'] <= 0) {
        form_set_error('distance', t('Distance must be a numeric value greater than zero.'));
        $error = TRUE;  
      }
    }    
    if ($error) { return; }
    
    $params = array(
      'alt'         => $this->config['format'],
      'v'           => 2,
      'safeSearch'  => $values['safe_search'],
    );
    
    if ($values['authors']) {
      $authors = array();
      
      // Clean up specified YouTube user names before query injection.
      foreach (explode(',', $values['authors']) as $author) {
        $authors[] = trim($author);
      }    
      $params['author'] = implode(',', $authors);
    }
    
    if ($values['tags']) {
      $tags = array();
      
      // Clean up specified tags before query injection.
      foreach (explode(',', $values['tags']) as $tag) {
        $tags[] = trim($tag);
      }      
      $params['tags'] = implode(',', $tags);
    }
    
    if ($values['use_location']) {
      $params['location']        = trim($latitude) . ',' . trim($longitude);
      $params['location-radius'] = trim($values['distance']) . $values['distance_unit'];
    }
    
    if ($values['query']) {
      $params['q'] = trim($values['query']);  
    }       
    
    $query = drupal_query_string_encode($params);
    $values['source'] = $this->config['search_url'] . "?$query";    
  }
}