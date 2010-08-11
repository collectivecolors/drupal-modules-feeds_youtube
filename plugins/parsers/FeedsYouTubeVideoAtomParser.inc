<?php
// $Id$
/**
 * @file
 *  Feeds parser class for YouTube.

/**
 * Class definition for YouTube Video Atom Parser.
 *
 * Parses Atom video feeds.
 */
class FeedsYouTubeVideoAtomParser extends FeedsSimplePieParser {
  
  // YouTube feed namespaces.
  
  const NAMESPACE_MEDIA = 'http://search.yahoo.com/mrss/';
  const NAMESPACE_APP   = 'http://www.w3.org/2007/app';
  const NAMESPACE_GD    = 'http://schemas.google.com/g/2005';
  const NAMESPACE_YT    = 'http://gdata.youtube.com/schemas/2007';

  /**
   * Parse the extra mapping sources provided by this parser.
   */
  protected function parseExtensions(&$item, $simplepie_item) {
    
    $date_parser = SimplePie_Parse_Date::get();
    
    unset($item['author_email']); // YouTube does not supply this information.
    
    // SimplePie is encoding my damn query variables.
    $item['url'] = urldecode($item['url']);
    
    // Get access control information.
    
    if ($control = $simplepie_item->get_item_tags(self::NAMESPACE_APP, 'control')) {
      $draft = $control[0]['child'][self::NAMESPACE_APP]['draft'][0]['data'];
      $state = $control[0]['child'][self::NAMESPACE_YT]['state'][0];
        
      $item['state_draft']   = ($draft == 'yes' ? 1 : 0);
      $item['state_name']    = $state['attribs']['']['name'];
      $item['state_reason']  = $state['attribs']['']['reasonCode'];
      $item['state_message'] = $state['data'];
    }
    if ($access = $simplepie_item->get_item_tags(self::NAMESPACE_YT, 'accessControl')) {
      $allowed = array();
      $denied  = array();
      
      foreach ((array)$access as $rule) {
        $action     = $rule['attribs']['']['action'];
        $permission = $rule['attribs']['']['permission'];

        if ($permission == 'allowed') {
          $allowed[] = $action;
        }
        else {
          $denied[] = $action;
        }
      }      
      $item['access_allowed'] = $allowed;
      $item['access_denied']  = $denied;
    }
    
    // Check if we can embed video.
    
    $noembed = $simplepie_item->get_item_tags(self::NAMESPACE_YT, 'noembed');
    $item['embed_allowed'] = (count($noembed) ? 0 : 1);
    
    // Get comment information.
    
    if ($comments = $simplepie_item->get_item_tags(self::NAMESPACE_GD, 'comments')) {
      $feedlink = $comments[0]['child'][self::NAMESPACE_GD]['feedLink'][0];
      
      $item['comment_count'] = $feedlink['attribs']['']['countHint'];
    }
    
    // Get video ratings.
    
    if ($rating = $simplepie_item->get_item_tags(self::NAMESPACE_GD, 'rating')) {
      $item['rating_average']    = $rating[0]['attribs']['']['average'];
      $item['rating_min']        = $rating[0]['attribs']['']['min'];
      $item['rating_max']        = $rating[0]['attribs']['']['max'];
      $item['rating_num_raters'] = $rating[0]['attribs']['']['numRaters'];
    }
    if ($like_dislike = $simplepie_item->get_item_tags(self::NAMESPACE_YT, 'rating')) {
      $item['rating_likes']    = $like_dislike[0]['attribs']['']['numLikes'];
      $item['rating_dislikes'] = $like_dislike[0]['attribs']['']['numDislikes'];  
    }
    
    // Get statistics.
    
    if ($stats = $simplepie_item->get_item_tags(self::NAMESPACE_YT, 'statistics')) {
      $item['stats_favorite'] = $stats[0]['attribs']['']['favoriteCount'];
      $item['stats_views']    = $stats[0]['attribs']['']['viewCount'];  
    }
    
    // Get other video information.
     
    // This information may not be in a SimplePie enclosure so we explicitly 
    // define it here.  I know, that is too bad because SimplePie's support
    // for nested tags is absolutely attrocious.  I plan to migrate this parser
    // to simple xml when I get the time. 
    
    if ($group = $simplepie_item->get_item_tags(self::NAMESPACE_MEDIA, 'group')) {
      $group_children = $group[0]['child'];
      if ($group_children) {
        // Media fields.
        if(isset($group_children[self::NAMESPACE_MEDIA]['title'])) {
          $item['media_title'] = $group_children[self::NAMESPACE_MEDIA]['title'][0]['data'];
        }
        if(isset($group_children[self::NAMESPACE_MEDIA]['credit'])) {
          $item['media_credit'] = $group_children[self::NAMESPACE_MEDIA]['credit'][0]['data'];
        }
        if(isset($group_children[self::NAMESPACE_MEDIA]['description'])) {
          $item['media_description'] = $group_children[self::NAMESPACE_MEDIA]['description'][0]['data'];
        }
        if(isset($group_children[self::NAMESPACE_MEDIA]['player'])) {
          $item['media_player'] = $group_children[self::NAMESPACE_MEDIA]['player'][0]['attribs']['']['url'];
        }
        if(isset($group_children[self::NAMESPACE_MEDIA]['thumbnail'])) {
          $thumbnail_links = array();          
          foreach ($group_children[self::NAMESPACE_MEDIA]['thumbnail'] as $thumb) {
            $thumbnail_links[] = $thumb['attribs']['']['url'];
          }
          $item['media_thumbnails'] = $thumbnail_links;
        }
        
        if(isset($group_children[self::NAMESPACE_MEDIA]['keywords'])) {
          $keywords = array();          
          foreach (explode(',', $group_children[self::NAMESPACE_MEDIA]['keywords'][0]['data']) as $keyword) {
            $keywords[] = trim($keyword);
          }
          $item['media_keywords'] = $keywords;
        }        
        if(isset($group_children[self::NAMESPACE_MEDIA]['category'])) {
          $item['media_category'] = $group_children[self::NAMESPACE_MEDIA]['category'][0]['data'];
        }
        
        // YouTube fields.
        if(isset($group_children[self::NAMESPACE_YT]['videoid'])) {
          $item['video_id'] = $group_children[self::NAMESPACE_YT]['videoid'][0]['data'];
        }
        if(isset($group_children[self::NAMESPACE_YT]['uploaded'])) {
          $item['video_uploaded'] = $date_parser->parse($group_children[self::NAMESPACE_YT]['uploaded'][0]['data']);
        }
        if(isset($group_children[self::NAMESPACE_YT]['duration'])) {
          $item['media_duration'] = $group_children[self::NAMESPACE_YT]['duration'][0]['attribs']['']['seconds'];
        }        
      }      
    }    

    // Extract standard media file fields out of the default enclosure.
        
    $enclosures        = $simplepie_item->get_enclosures();
    $default_enclosure = NULL;
    
    if (count($enclosures)) {
      foreach ($enclosures as $enc) {
        // We are looking for a flash video that we can play.
        if ($enc->get_type() == 'application/x-shockwave-flash') {
          $default_enclosure = $enc;
          break;
        }
      }      
    }    
    if (is_null($default_enclosure)) {
      // There is nothing more to process.
      return;
    }
    
    $item['media_link']       = $default_enclosure->get_link(); // Link
    $item['media_type']       = $default_enclosure->get_type(); // Text
    $item['media_expression'] = $default_enclosure->get_expression(); // Text
  }

  /**
   * Add the extra mapping sources provided by this parser.
   */
  public function getMappingSources() {
    $map = parent::getMappingSources();
    
    unset($map['author_email']); // Not used.   
    
    return $map + array(      
      'state_draft' => array(
        'name' => t('W3 App: Draft'),
        'description' => t('This will be boolean true or false.'),
      ),
      'state_name' => array(
        'name' => t('YouTube: State Name'),
        'description' => t('YouTube:state name field.'),
      ),
      'state_reason' => array(
        'name' => t('YouTube: State Reason'),
        'description' => t('YouTube:state reasonCode field.'),
      ),
      'state_message' => array(
        'name' => t('YouTube: State Message'),
        'description' => t('YouTube:state message field.'),
      ),
      
      'access_allowed' => array(
        'name' => t('YouTube: AccessControl Allowed'),
        'description' => t('An array of YouTube:accessControl allowed actions.'),
      ),
      'access_denied' => array(
        'name' => t('YouTube: AccessControl Denied'),
        'description' => t('An array of YouTube:accessControl denied actions.'),
      ),      
    
      'embed_allowed' => array(
        'name' => t('Embed allowed?'),
        'description' => t('This will be boolean true or false.'),
      ),
      
      'comment_count' => array(
        'name' => t('Google: Comments Count'),
        'description' => t('Google:comments countHint field.'),
      ),
      
      'rating_average' => array(
        'name' => t('Google: Rating Average'),
        'description' => t('Google:rating average field.'),
      ),
      'rating_min' => array(
        'name' => t('Google: Rating Min'),
        'description' => t('Google:rating min field.'),
      ),      
    
      'rating_max' => array(
        'name' => t('Google: Rating Max'),
        'description' => t('Google:rating max field.'),
      ),
      'rating_num_raters' => array(
        'name' => t('Google: Rating Number of Raters'),
        'description' => t('Google:rating numRaters field.'),
      ), 
      
      'rating_likes' => array(
        'name' => t('YouTube: Rating Likes'),
        'description' => t('YouTube:rating numLikes field.'),
      ),
      'rating_dislikes' => array(
        'name' => t('YouTube: Rating Dislikes'),
        'description' => t('YouTube:rating numDislikes field.'),
      ),      
    
      'stats_favorite' => array(
        'name' => t('YouTube: Statistics Favorites'),
        'description' => t('YouTube:statistics favorites field.'),
      ),
      'stats_views' => array(
        'name' => t('YouTube: Statistics Views'),
        'description' => t('YouTube:statistics views field.'),
      ),      
    
      'video_id' => array(
        'name' => t('YouTube: Videoid'),
        'description' => t('YouTube:videoid field.'),
      ),
      'video_uploaded' => array(
        'name' => t('YouTube: Uploaded Date'),
        'description' => t('YouTube:uploaded field.'),
      ),    
    
      'media_link' => array(
        'name' => t('Media: Link'),
        'description' => t('Media:content url field.'),
      ),
      'media_player' => array(
        'name' => t('Media: Player'),
        'description' => t('Media:player url field.')
      ),
      'media_thumbnails' => array(
        'name' => t('Media: Thumbnails'),
        'description' => t('An array of media:thumbnail url fields.'),
      ),
      
      'media_type' => array(
        'name' => t('Media type'),
        'description' => t('File type of media.'),
      ),
      'media_title' => array(
        'name' => t('Media: Title'),
        'description' => t('Media:title field.'),
      ),
      'media_description' => array(
        'name' => t('Media: Description'),
        'description' => t('Media:description field.'),
      ),
      'media_keywords' => array(
        'name' => t('Media: Keywords'),
        'description' => t('An array of media:keyword fields.'),
      ),
      
      'media_expression' => array(
        'name' => t('Media expression'),
        'description' => t('Media expression: full, sample, nonstop.'),
      ),
      'media_duration' => array(
        'name' => t('Media duration'),
        'description' => t('Media duration in seconds.'),
      ),
      
      'media_category' => array(
        'name' => t('Media: Category'),
        'description' => t('Media:category field.'),
      ),
      'media_credit' => array(
        'name' => t('Media: Credit'),
        'description' => t('Media:credit field.'),
      ),     
    );
  }
}
