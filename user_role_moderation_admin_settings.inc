<?php
// $Id: user_role_moderation_admin_settings.inc,v 1.11 2010/05/26 05:23:30 shamlin Exp $

function user_role_moderation_admin() {
  $form = array();
  $form['URM_list_max'] = array(
    '#type' => 'textfield',
    '#title' => t('List number of users per page'),
    '#default_value' => variable_get('URM_list_max', 20),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('List number of users per page.'),
    '#required' => TRUE,
  );
  $form['URM_lock'] = array(
    '#type' => 'checkbox',
    '#attributes' => array(
      'disabled' => 'disabled',
    ),
    '#title' => t('Enable user lock'),
    '#default_value' => variable_get('URM_lock', 1),
    '#description' => t('Enable user locking when moderator viewing the user 
      comments. (Currently must be enabled)'),
  );
  $form['URM_lock_expiry_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Set user lock expiry time'),
    '#default_value' => variable_get('URM_lock_expiry_time', 120),
    '#size' => 4,
    '#maxlength' => 4,
    '#description' => t('Set the user lock expiry time in seconds'),
    '#required' => TRUE,
  );
  $roles = user_role_moderation_get_user_role();
  $form['URM_default_role'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select the following roles to appear on moderation list'),
    '#default_value' => variable_get('URM_default_role', 'none'),
    '#options' => $roles,
    '#description' => t('These roles are to be appeared on moderation list'),
    '#required' => TRUE,
  );
  $roles = user_role_moderation_get_user_role();
  $form['URM_target_role'] = array(
    '#type' => 'radios',
    '#title' => t('Assign this role to user when user is approved'),
    '#default_value' => variable_get('URM_target_role', 'none'),
    '#options' => $roles,
    '#description' => t('Assign this role to user when user is approved by 
      moderator'),
    '#required' => TRUE,
  );
  $reason = "SPAM\nAbusive\nNot on topic";
  $form['URM_reason'] = array(
    '#type' => 'textarea',
    '#title' => t('Define reasons why user is banned by moderator'),
    '#size' => 120,
    '#default_value' => variable_get('URM_reason', $reason),
    '#description' => t('Enter reasons, please seperate each reason by
      new line. This provides moderators options to choose when banning 
      a user. (Note: Please disable WYSIWYG editor for this field before 
      saving this setting.)'),
    '#required' => TRUE,
  );
  $theme = user_role_moderation_jquery_ui_themes();
  $form['URM_theme'] = array(
    '#type' => 'select',
    '#title' => t('Select the JQuery UI theme'),
   '#default_value' => variable_get('URM_theme', 'base'),
    '#options' =>  $theme,
    '#description' => t('Select the JQuery UI theme'),
    '#required' => TRUE,
  );
  
  return system_settings_form($form);
}

/**
 * Return all user roles in the role table
 * Prepend a role in the array 'none' => 'No role'
 * 
 * @return array of roles' name
 */
function user_role_moderation_get_user_role(){
  $sql = "SELECT * FROM {role} WHERE rid <> 1 AND rid <> 2";
  $result = db_query($sql);
  $name['none']= 'No role';
  while ($roles = db_fetch_object($result)) {
    $rid = $roles->rid;
    $name[$rid] = $roles->name;
  }
  return $name;
}

/**
 * Return available JQuery UI themes from the JQuery UI folder 
 * @return 
 */
function user_role_moderation_jquery_ui_themes(){
  $search_dir = drupal_get_path('module', 'jquery_ui').'/jquery.ui/themes';
  $exclude = array('.', '..', 'CVS');
  
  $dir = file_scan_directory($search_dir, '.*', $exclude , 0, FALSE);
  
  if(is_array($dir)){
    foreach($dir as $key => $val) {
      $output[$val->basename] = $val->basename; 
    }
    return $output;
  }else{
    return FALSE;
  }
}
