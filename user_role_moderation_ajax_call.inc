<?php
// $Id: user_role_moderation_ajax_call.inc,v 1.23 2010/05/27 04:39:08 shamlin Exp $

/**
 * This function handles user approval ajax call.
 * Ajax response format: uid=valid|invalid & 
 * status=approve_successful|approve_failed|
 * Ajax get method:
 * 
 * @param object $uid [optional]
 * @return Ajax response message 
 */
function user_role_moderation_approve_user($uid) {
  global $user;
  if(user_role_moderation_ajax_call($uid)) {
    if(user_role_moderation_is_unmoderated_user($uid)) {
      $target_role = variable_get('URM_target_role', '');
  
      if($target_role) {
        //Update if only no result
        $sql = "INSERT INTO {users_roles}
          (uid, rid) VALUES (%d, %d)"; 
        $success = db_query($sql, $uid, $target_role);

        if($success) {
          $out = 'uid=valid&status=approve_successful';
          
          //trigger _user_mail_notify from drupal core.
          $user_obj = user_load($uid);
          $op = 'status_activated';
          _user_mail_notify($op, $user_obj);
           
          //trigger flag - set it to reviewed  
          $ucm_flag = variable_get('UCM_flag', '');
          if($ucm_flag) {
            $flag = flag_get_flag($ucm_flag);
            $s = "SELECT cid FROM {comments} WHERE uid='%d'";
            $r = db_query($s, $uid);
            while ($rs = db_fetch_object($r)) {
              $cid = $rs->cid;
             
              //Publish comment
              $upd = "UPDATE {comments}
                SET status = '0'
                WHERE cid = '%d'";
              db_query($upd, $cid);

              watchdog('URM - approve', 
                'Published comment %cid.', 
                array('%cid' => $cid)
              );
              
              //Set comment flag to reviewed status
              if(!$flag->is_flagged($cid)) {     
                $flag->flag('flag', $cid);   
              }                   
            }
          }  
        } else {
          $out = 'uid=valid&status=approve_failed';
        }          
      } else {
          $out = 'uid=valid&status=approve_failed';
      }
    } else {
        $out = 'uid=invalid&status=approve_failed';
    } 
  } else {
    $out = 'uid=invalid&status=approve_failed'; 
  }
  print t($out);
}

/**
 * Ban the user  - does several actions:
 * <ul>
 *   <li>Blocks the user</li>
 *   <li>Notifies the user via email that they are blocked</li>
 *   <li>destroys their session</li>
 * </ul>
 * 
 * @param object $uid
 * @param object $reason_id [optional]
 * @return success or fail string
 */
function user_role_moderation_ban_user($uid, $reason_id = '') {
  if(user_role_moderation_ajax_call($uid)) {
    //Block user
    $sql = "UPDATE {users} SET status='0' WHERE uid='%d'";
            
    $success = db_query($sql, $uid);
    if($success) {
      //Insert moderator's selected reason into reason to ban table
      if(user_role_moderation_insert_comment($uid, $reason_id)) {
        $out = 'uid=valid&status=ban_successful&reason=' . $reason_id;
        
        //_user_mail_notify is drupal core function...
        $user_obj = user_load($uid);
        $op = 'status_blocked';
        _user_mail_notify($op, $user_obj);
        sess_destroy_uid($uid);
      } else {
        $out = 'uid=valid&status=ban_failed';
      }
    } else {
      $out = 'uid=valid&status=ban_failed';
    }
  } else {
    $out = 'uid=invalid&status=ban_failed';
  }
  print t($out);
}

/**
 * This function checks all conditions before allowing
 * moderator updates the table
 * @param object $uid [optional]
 * @return TRUE / FALSE
 */
function user_role_moderation_ajax_call($uid) {
  global $user;
  //Guard statement - to make user has not been assigned to any role
  if(!user_role_moderation_is_unmoderated_user($uid)) {
    return FALSE;  
  }
  
  //Make sue this user is not locked by another moderator
  $lockedUser = user_role_moderation_user_is_locked($uid);
  
  if (!$lockedUser) {
    //Insert lock to table
    user_role_moderation_insert_locked_ssesion($uid);
    return TRUE;
  } else {  
    if ($lockedUser[locked_datetime] + 
      variable_get('URM_lock_expiry_time', '120') > time()) {
      
      if($lockedUser['locked_by'] == $user->uid) {
        //Renew my locked session
        //echo "renewing_session-".$uid;
        user_role_moderation_renew_locked_session($uid);
        return TRUE;  
      } else {
        //Return false
        //echo "session is locked by".$item['locked_by'];
        return FALSE;
      }
    } else {
      //Return true
      user_role_moderation_remove_locked_ssesion($uid);
      user_role_moderation_insert_locked_ssesion($uid);
      return TRUE;
    }
  }
}

/**
 * This function uses Ajax to test the user uid status
 * Ajax response format: key=val&key2=val2
 * update=success|fail&locked=true|false&
 * @param object $uid
 * @return Ajax response string
 */
function user_role_moderation_ajax_lock($uid) {
  global $user;
  //Guard statement - to make user user is not being assigned a role
  if(!user_role_moderation_is_unmoderated_user($uid)) {
    $out = 'uid=invalid';  
  } else {
    $out = 'uid=valid'; 
    
    $lockedUser = user_role_moderation_user_is_locked($uid);
    
    if (!$lockedUser) {
      //Insert lock to table
      user_role_moderation_insert_locked_ssesion($uid);
      $out .= '&status=new_session';  
  
    } else {  
  
      if($lockedUser[locked_datetime] + variable_get('URM_lock_expiry_time', '120') > time()) {
        if($lockedUser['locked_by'] == $user->uid) {
          //Renew my locked session
          user_role_moderation_renew_locked_session($uid);
          $out .= '&status=renew_session';  
        } else {
          //Return false
          //echo "session is locked by".$item['locked_by'];
          $out .= '&status=session_is_locked';  
        }
  
      } else {
        //Return true
        user_role_moderation_remove_locked_ssesion($uid);
        user_role_moderation_insert_locked_ssesion($uid);
        $out.= '&status=overwrite_new_session';
      }
    }  
  }
  
  print t($out);
}

/**
 * This function unlock locked user
 * Ajax response format: key=val&key2=val2
 * update=success|fail&locked=true|false&
 * 
 * @param object $uid
 * @return 
 */
function user_role_moderation_ajax_unlock($uid) {
  $out = '';

  global $user;
  // check if user is locked
  $userLocked = user_role_moderation_user_is_locked($uid);
  if (!$userLocked) {
    $out .= 'status=user_not_locked'; 
    print t($out);
    //return;
  }
  
  // make sure you are the locker
  if($userLocked['locked_by'] == $user->uid) {
    // remove lock
    user_role_moderation_remove_locked_ssesion($uid);
    $out .= 'status=lock_removed';  
  } 
  else {
    // fail
    $out .= 'status=locked_by_another_user';  
  }
  print t($out);
}

/**
 * Check to see if the uid is valid
 * 
 * @param object $uid the user ID
 * @return 
 */
function user_role_moderation_is_valid_user($uid) {
  // User's UID must exist
  // exclude uid 0, 1
  $sql = "SELECT *
    FROM {users} AS u 
    WHERE u.uid = '%d'
    AND u.uid > 1";
  $result = db_query($sql, $uid);

  if (count($result) == '1') {
    return db_result($result);
  } else {
    return FALSE;
  }
}

/**
 *  This function check if user has never been moderated
 * @param object $uid [optional]
 * @return 
 */
function user_role_moderation_is_unmoderated_user($uid) {
  $default_role = variable_get('URM_default_role',0);
  if(is_array($default_role)) {
    foreach($default_role as $key =>$val) {
      if($val == 'none') {
        $sql_filter = ' ur.rid IS NULL ';
      } else {
        $all_temp[$key]=$val;  
      }
    }
  }
  if(is_array($all_temp)) {
    $list = implode(',', $all_temp);  
  }
  if($list) {
    if($sql_filter) {
      $sql_filter .= ' OR ';
    }
    $sql_filter .= ' ur.rid IN (' . $list . ')';
  }
  $sql_filter = ' AND (' . $sql_filter . ')';

  if ($uid) {
    $sql = "SELECT *
      FROM {users} AS u
      LEFT JOIN {users_roles} AS ur
      ON u.uid = ur.uid 
      WHERE u.uid = '%d'
      AND u.status = 1
      AND u.uid > 1 " . $sql_filter;
    $result = db_query($sql, $uid);
      
    if (count($result) == 1 ) {
      $out = db_result($result);
    } else {
      $out = 0;
    }
  } else {
    $out = 0;
  }

  if ($out == 0) {
      return FALSE;
  } else {
      return $out;
  }
}

/**
 * This function check is user is locked
 * @param object $uid [optional]
 * @return 
 */

function user_role_moderation_user_is_locked($uid) {
  //Return moderator UID who loocked the user
  $sql = "SELECT *
    FROM {user_role_moderation} 
    AS urm
    WHERE urm.uid='%d'
    AND urm.uid > 1 ";
    
   $result = db_query($sql, $uid);
    //$out=array();
    while ($row = db_fetch_object($result)) {
      $out['uid'] = $row->uid;      
      $out['locked_by'] = $row->locked_by;      
      $out['locked_datetime'] = $row->locked_datetime;      
      $out['status'] = $row->status;
      $out['total']= count($row);      
    }

    if(count($row) == 1 ) {
      return $out;  
    } else {
      return FALSE;
    }
}

/**
 * This function creates a new lock session
 * @param object $uid [optional]
 * @return 
 */

function user_role_moderation_insert_locked_ssesion($uid) {
  global $user;
  $now = time();  
  $sql = "INSERT INTO {user_role_moderation}
    (uid, locked_by, locked_datetime, status)
    VALUES ('%d', '%d', '%d',  '%d')";

  if(db_query($sql, $uid, $user->uid , $now ,1)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 * This function renew locked session
 * 
 * @param object $uid
 * @return 
 */
function user_role_moderation_renew_locked_session($uid) {
  global $user;
  $now = time();  

  $sql = "UPDATE {user_role_moderation}
    SET locked_datetime='%d'
    WHERE uid='%d' AND locked_by='%d'";
    
  if(db_query($sql, $now, $uid, $user->uid)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 * This function unlocks locked session
 * 
 * @param object $uid
 * @return 
 */
function user_role_moderation_remove_locked_ssesion($uid) {
  //Unlocked session locked by another moderator
  $sql = "DELETE FROM {user_role_moderation} WHERE uid= '%d'";
  if(db_query($sql, $uid )) {
    return TRUE;
  } else {
    return FALSE;
  }  
}

/**
 * This function inserts reason to ban into the URM moderator comment table
 * 
 * @param object
 * @return 
 */
function user_role_moderation_insert_comment($uid, $reason_id='') {
  global $user;
  
  if($uid=='' || $reason_id=='') {
    return FALSE;
  } else {
    $now = time();
    $reasons = chop(variable_get('URM_reason', ''));
    $reason = explode('\n', $reasons);
    
    $comment = $reason[$reason_id];
      
    $sql = "INSERT INTO {user_role_moderation_comment}
      (uid, moderator_id, datetime, comment)
      VALUES ( '%d', '%d', '%d' , '%s' )";
        
    if(db_query($sql, $uid, $user->uid, $now, $comment)) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
}