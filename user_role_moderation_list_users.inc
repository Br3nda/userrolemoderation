<?php

/**
 * List users on the first post moderation screen
 * @return
 */
function user_role_moderation_list_users() {
  // add js + css
  jquery_ui_add(array(
    'ui.core',
    'effects.core',
    'ui.accordion'
  ));
  drupal_add_css(drupal_get_path('module', 'jquery_ui') .'/jquery.ui/themes/' . variable_get('URM_theme', 'base') .'/ui.all.css');

  //Include js only to this function
  drupal_add_css(drupal_get_path('module', 'user_role_moderation') .'/user_role_moderation.css');
  drupal_add_js(drupal_get_path('module', 'user_role_moderation') .'/user_role_moderation.js');

  $default_role = variable_get('URM_default_role', 0);

  foreach ($default_role as $key => $val) {
    if ($val == 'none') {
      $sql_filter = ' r.rid IS NULL ';
    }
    else {
      $all_temp[$key] = $val;
    }
  }

  if (is_array($all_temp)) {
    $list = implode(',', $all_temp);
  }

  if ($list) {
    if ($sql_filter) {
      $sql_filter .= ' OR ';
    }
    $sql_filter .= ' r.rid IN (' . $list . ')';
  }

  $sql_filter = '(' . $sql_filter . ')';

  $sql= "SELECT
    count(c.cid) as tt,
    u.uid as uid,
    u.name AS uname,
    u.created AS ucreated,
    r.rid,r.name AS rname
    FROM {users} u
    LEFT JOIN {users_roles} AS ur
    ON u.uid = ur.uid
    LEFT JOIN {role} r
    ON ur.rid = r.rid
    LEFT JOIN {comments} AS c
    On u.uid = c.uid
    WHERE " . $sql_filter . "
    AND u.uid <> 0
    AND u.status = 1
    GROUP BY u.uid , u.name, u.created, r.rid, r.name
    HAVING count(c.cid) > 0";
 

//   $result = db_query($sql);
  $result = pager_query($sql, variable_get('URM_list_max', 20));
  while ($users = db_fetch_object($result)) {
    // get user details (XSS filtered)
    $uid = $users->uid;
    $uname = check_plain($users->uname);
    $cid = $users->cid;
    $ucreated = $users->ucreated;

    $data['username'] = $uname;
    $data['uid'] = $uid;

    // get the user comments array
    $user_comments = user_role_moderation_show_comments($uid);
    $list .=
      '<h3 id="user_uid-' . $uid . '">' .
        '<span class="username">' . $uname . '.</span>' .
        '<span class="error-reason"></span>' .
        '<span class="ajax-spinner"></span> ' .
        '<span class="date">' .
          t('Registered %timeago.', array('%timeago' =>  user_role_moderation_ago($ucreated))) .
        '</span>  ' .
        '<span class="total-comment">' .
        t('@count comments.', array('@count' => $user_comments['total'])) .
        '</span>' .
      '</h3>';

    $list .= '<div>';
    $list .= $user_comments['output'];

    $list .=
      '<p>' .
        l('<span class="ui-icon ui-icon-circle-close"></span>' .
          '<span class="text">' . t('Ban @name', array('@name' => $uname)) . '</span>' .
          '<span class="ui-icon ui-icon-right ui-icon-triangle-1-s"></span>',
          'colorbox/form/user_role_moderation_ban_user_form',
          array(
            attributes => array(
              'id' => 'ban_user-'. $uid,
              'class' => 'ban colorbox-form ui-state-default ui-corner-all',
              'title' => t('Ban the user @name.', array('@name' => $uname)),
              ),
            html => TRUE,
            )
          ) .
        l('<span class="ui-icon ui-icon-circle-check"></span>' .
          '<span class="text">'. t('Approve @name.', array('@name' => $uname )). '</span>',
          'action/approve/' . $uid,
          array(
            attributes => array(
              'id' => 'approve_user-' . $uid,
              'class' => 'approve colorbox-form ui-state-default ui-corner-all',
              'title' => t('Approve the user @name.', array('@name' => uname,))
              ),
            html => TRUE,
            )
          ) .
      '</p>';
    $list .=
      '<div style="display: none;">'.
        drupal_get_form('user_role_moderation_ban_user_form', $data) .
      '</div>';
    $list .= '<div class="clear"></div></div>';
  }

  if ($list) {
    return '<div id="accordion">'. $list .'</div>';
  }
  return t('There are no users pending for moderation.');
}

/**
 * This function displays individual user comments
 * @param object $uid
 * @return
 */
function user_role_moderation_show_comments($uid) {
  if ($uid) {
    $sql= "SELECT c.comment AS ucomment,
      c.*, n.*
      FROM {comments} c
      LEFT JOIN {node} n
      ON c.nid = n.nid
      WHERE c.uid = %d
      AND n.status = '1'
      ORDER BY c.timestamp DESC
      LIMIT 20";

    $result = db_query($sql, $uid);
    $list = '<ul class="comments">';
    $num_row = 0;
    while ($rs = db_fetch_object($result)) {
      $comment = $rs->ucomment;
      $cid = $rs->cid;
      $nid = $rs->nid;

      $title = $rs->title;
      $num_row++;

      $list .= '<li>';

      // load the node + the comment objects
      $c = _comment_load($cid);
      $n = node_load($c->nid);

      // title + the comment (XSS filtered)
      $list .= '<h5 class="comment-title">'. check_plain($n->title) .'</h5>';
      $list .= '<p>'. check_markup($comment, $c->format) .'</p>';

      $list .= '<ul class="comment-button"><li>'.
          l('Delete comment', 'comment/delete/'. $cid, array(
              attributes => array(
                'id' => 'cid_delete_'. intval($cid),
                'class' => 'colorbox-form ui-state-default ui-corner-all',
                'title' => 'Delete the comment',
                )
              )
            ) .
          '</li>'.
          '<li>' .
          l('Edit comment', 'comment/edit/'. $cid, array(
              attributes => array(
                'id' => 'cid_edit_'. $cid,
                'class' => 'colorbox-form ui-state-default ui-corner-all',
                'title' => 'Edit the comment',
                )
              )
            ) .
          '</li>'.
          '<li>'.
          l('Reply', 'comment/reply/'. $nid .'/'. $cid, array(
              attributes => array(
                'id' => 'cid_reply_'. $cid,
                'class' => 'colorbox-form ui-state-default ui-corner-all',
                'title' => 'Reply to the comment',
                )
              )
            ) .
          '</li>'.
        '</ul>';

      $list .=  '</li>';
    }
    $list .= '</ul>';

    $out['output'] = $list;
    $out['total'] = $num_row;
    return $out;
  }
  else {
    return FALSE;
  }
}

/**
 * This function generate ban user form items (elements) for each comment
 * @param object $form_state
 * @param object $user_info
 * @return
 */
function user_role_moderation_ban_user_form($form_state, $user_info) {
  $reasons = chop(variable_get('URM_reason', ''));
  $reason = explode("\n", $reasons);

  $form_id = 'ban_form-'. $user_info['uid'];
  $ban_uid = 'ban_uid_'. $user_info['uid'];

  $form = array(
    '#type' => 'form',
    '#method' => 'post',
    '#attributes' => array( 'class' => 'ban_forms'),
    '#id' => t($form_id),
    '#tree' => TRUE,
  );
  $form[$ban_uid] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the reason why do you ban '.
      $user_info['username'] . ' . '),
    '#attributes' => array( 'class' => '', 'id' => ''),
    '#tree' => TRUE,
  );
  $form[$ban_uid]['reason'] = array(
    '#type' => 'radios',
    '#title' => t('Reasons'),
    '#default_value' => '',
    '#attributes' => array( 'class' => 'ban_reasons', 'id' => ''),
    '#options' =>  $reason ,
    '#name' => 'reason[]',
    '#description' => t('Select one of the reason why do you ban '.
      $user_info['username'] .' . '),
  );
  $form[$ban_uid]['submit'] = array(
    '#type' => 'button',
    '#attributes' => array('class' => 'ui-state-default ui-corner-all'),
    '#name' => 'step',
    '#id' => 'confirm_ban-' . $user_info['uid'],
    '#value' => t('Ban ' . $user_info['username']),
  );
  $form[$ban_uid]['cancel'] = array(
    '#type' => 'button',
    '#attributes' => array( 'class' => 'ui-state-default ui-corner-all'),
    '#name' => 'step',
    '#id' => 'cancer_ban-' . $user_info['uid'],
    '#value' => t('Cancel'),
  );

  return $form;
}

/**
 * Convert timestamp to human readable date format
 *
 * @param object $timestamp
 * @return pretty formatted time ago string
 */
function user_role_moderation_ago($timestamp) {
  $difference = time() - $timestamp;
  $periods = array(
    'second',
    'minute',
    'hour',
    'day',
    'week',
    'month',
    'year',
    'decade',
  );
  $lengths = array('60', '60', '24', '7', '4.35', '12', '10');
  for ($j = 0; $difference >= $lengths[$j]; $j++) {
    $difference /= $lengths[$j];
  }
  $difference = round($difference);
  if ($difference != 1) {
    $periods[$j] .= 's';
  }
  $text = $difference . ' ' . $periods[$j] . ' ago';
  return $text;
}