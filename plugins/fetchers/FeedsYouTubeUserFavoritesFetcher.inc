<?php
// $Id$

/**
 * @file
 * Home of the FeedsYouTubeUserFavoritesFetcher.
 */

/**
 * Fetches YouTube videos that are favorites of a user.
 *   
 * This class basically exists to make creating feed nodes that fetch YouTube 
 * videos easier for the end user.
 */
class FeedsYouTubeUserFavoritesFetcher extends FeedsHTTPFetcher {
  
  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    $defaults = parent::configDefaults();
    unset($defaults['auto_detect_feeds']);
    
    $defaults['favorites_url'] = 'http://gdata.youtube.com/feeds/api/users/%username/favorites';
    $defaults['format']     = 'atom';
       
    return $defaults;
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = parent::configForm($form_state);
    unset($form['auto_detect_feeds']);
    
    $form['favorites_url'] = array(
      '#type' => 'textfield',
      '#title' => t('YouTube feed endpoint'),
      '#description' => t('You may use <b>%username</b> as a placeholder for the YouTube user name.  If you change this URL be sure that the same query parameters are used.  If you empty this input then submit the default URL will be used'),
      '#default_value' => $this->config['favorites_url'],
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
    
    if (!$values['favorites_url']) { // TODO: This should really be a check for a valid URL.
      $defaults = $this->configDefaults();
      $values['favorites_url'] = $defaults['favorites_url'];
    }
  }
  
  /**
   * Expose source form.
   */
  public function sourceForm($source_config) {
    
    $form['username'] = array(
      '#type' => 'textfield',
      '#title' => t('YouTube user name'),
      '#description' => t('This feed will import the favotite videos of the specified user.'),
      '#default_value' => ($source_config['username'] ? $source_config['username'] : ''),
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
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    
    if (!$values['username']) { // TODO: Validate user name correctly.  Valid username and user exists.
      form_set_error('username', t('A YouTube username is required to import favorite videos.'));
      return;  
    }
    
    $params = array(
      'alt'        => $this->config['format'],
      'v'          => 2,
      'safeSearch' => $values['safe_search'],
    );

    $feed_url = strtr($this->config['favorites_url'], array('%username' => $values['username']));
    
    $query = drupal_query_string_encode($params);
    $values['source'] = "$feed_url?$query";    
  }
}