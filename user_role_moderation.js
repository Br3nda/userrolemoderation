/**
 * Global DOM ready JS
 * 
 * @param {Object} "#accordion"
 */
$(document).ready(function(){
  // build the accordion (only if it exists)
  if ($("#accordion").length > 0) {
    $("#accordion").accordion({
      autoHeight: false,
      collapsible: true,
      active: false,
      event: ""
    });
  }
  
  // 60 second auto refresh (only if there is no open accordion panes),
  // if a pane is open, defer refresh for another 60 seconds
  setTimeout('doRefresh()', 60000);
  
  // change document title to correspond to the number of items on the TODO list
  document.title = '(' + countPanes() + ') Role moderation';
  
  // Bind function to ban buttons
  $('form.ban_forms input:submit:[value^=Ban]').bind('click',function(){
    var this_id = $(this).attr("id");
    var uid = this_id.split("-").pop();
    return confirmBanUser(uid);
  });
  
  // Bind function to cancel buttons
  $('form.ban_forms input:submit:[value^=Cancel]').bind('click',function(){
    $.fn.colorbox.close();
    var this_id = $(this).attr("id");
    var uid = this_id.split("-").pop();      
    enableButtons(uid);
    return false;
  });

  // Unlock the previous user  
  $('#accordion').bind('accordionchange', function(event, ui) {
    if ($(ui.oldHeader).length > 0 && $(ui.newHeader).length > 0) {
      var accordionId = $(ui.oldHeader).attr("id");
      //var accordionIndex = $(ui.oldHeader + "h3").index(this);
      var uid = accordionId.split("-").pop();
      unLockRow(accordionId, uid, -1); 
    }
  });
  
  // Check if accordion is locked by another moderator
  $("#accordion h3").click(function(){
    var accordionId = $(this).attr("id");
    var accordionIndex = $("#accordion h3").index(this);
    var uid = accordionId.split("-").pop();
 
    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");

    // if row is active, then it is expanded and needs to be
    // unlocked and collapsed
    if ($(this).hasClass("ui-state-active")) {
       //alert(accordionId + uid + accordionIndex);
      unLockRow(accordionId, uid, accordionIndex);
    }
    // if row is not active, then it is collapsed and needs to be
    // locked and opened
    else {
      lockRow(accordionId, uid, accordionIndex);
    }
  });
  
  // Approve user, no confirmation is required
  // ID of <a> = approve_user-[uid]
  $("#accordion .approve").click(function(){
    var uid = $(this).attr("id").split("-").pop();
    var accordionId = "user_uid-" + uid;

    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");
    
    // check if this button has already been clicked,
    // if so, do not send another request
    if ($(this).hasClass("ui-state-disabled")) {return false;}
    
    // This send AJAX to approve user
    approveUser(uid);
    
    // disable buttons to stop user getting click happy
    disableButtons(uid);

    return false;
  });

  // Ban user, a popup is required to select the ban
  // reason, use jquery UI dialog
  $("#accordion .ban").click(function(){
    var uid = $(this).attr("id").split("-").pop();
    var accordionId = "user_uid-" + uid;
    
    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");
    
    // check if this button has already been clicked,
    // if so, do not send another request
    if ($(this).hasClass("ui-state-disabled")) {return false;}
    
    // This pop up confirmation form with reason for moderator to select
    banUser(uid);
    
    // disable buttons to stop user getting click happy
    disableButtons(uid);
    
    return false;
  });
});

/**
 * 60 second auto refresh (only if there is no open accordion panes),
 * if a pane is open, defer refresh for another 60 seconds
 */
function doRefresh() {
  // there is a pane open, reset the timer
  if (countOpenPanes() > 0) {
    // reset the timer
    setTimeout('doRefresh()', 60000);
  }
  // there is no pane open, reload the page
  else {
    document.location.reload();
  }
}

/**
 * Helper function to count the number of open panes
 */
function countOpenPanes() {
  // check if a accordion pane is open
  var panesOpen = 0;
  $("#accordion .ui-accordion-content").each(function(){
    if ($(this).css("display") == "block") {
      panesOpen++;
    }
  });
  return panesOpen;
}

/**
 * Helper function to count the number of panes
 */
function countPanes() {
  // check if a accordion pane is open
  var panesOpen = 0;
  $("#accordion .ui-accordion-content").each(function(){
    panesOpen++;
  });
  return panesOpen;
}

/**
 * Call the back end to attempt to create the lock
 * 
 * @param {Object} uid the user id you are trying to lock
 */
function lockRow (accordionId, uid, rowId) {
  // fire AJAX
  $.ajax({
    url: "/admin/moderation/user-role/action/lock/" + uid,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      // was user valid?
      var uidValid = data.split("&")[0].split("=").pop();
      if (uidValid != "valid") {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("uid was not valid !");
        return false;
      }
      // was locking successful?
      var lockingStatus = data.split("&")[1].split("=").pop();
      if (lockingStatus == "session_is_locked") {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("session is locked!");
        return false;
      }
      $("#" + accordionId).removeClass("ui-state-disabled");
      $("#" + accordionId + " .error-reason").html("");
      if (rowId >= 0) {
        $("#accordion").accordion("activate", rowId);
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      alert ("AJAX error");
      return false;
    }
  });
}

/**
 * Call the back end to attempt to remove the lock
 * 
 * @param {Object} uid the user id you are trying to lock
 */
function unLockRow (accordionId, uid, rowId) {
  // fire AJAX
  $.ajax({
    url: "/admin/moderation/user-role/action/unlock/" + uid,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      // was unlocking successful?
      var unLockingStatus = data.split("&")[0].split("=").pop();
      if (unLockingStatus == "lock_removed") {
        $("#" + accordionId).removeClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("");
        // collapse row if needed
        if (rowId >= 0) {
          $("#accordion").accordion("activate", rowId);
        }
      }
      // lock was not removed, display error
      else {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("error - " + unLockingStatus);
      }
      
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      alert ("AJAX error");
      return false;
    }
  });
}

/**
 * Add disabled classes to both buttons for this user
 * 
 * @param {Object} uid
 */
function disableButtons(uid){
  $("#ban_user-" + uid + ", #approve_user-" + uid)
    .addClass("ui-state-disabled"); 
}

/**
 * Remove disabled classes to both buttons for this user
 * 
 * @param {Object} uid
 */
function enableButtons(uid){
  $("#ban_user-" + uid + ", #approve_user-" + uid)
    .removeClass("ui-state-disabled"); 
}

/**
 * 
 * 
 * @param {Object} uid
 */
function approveUser(uid) {  
  // Fire Ajax
  // If successful - return true, remove current row
  // If failed - display errro message, enable buttons
  $.ajax({
    url: "/admin/moderation/user-role/action/approve/" + uid,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // remove spinner class on accordion row to show AJAX running
      // $("#" + accordionId).removeClass("ui-state-ajax");
      
      // was callback successful?
      var callbackUid = data.split("&")[0].split("=").pop();
      var callbackStatus = data.split("&")[1].split("=").pop();
      
      //Expected data uid=valid&status=approve_successful
      if (callbackUid == "valid" && callbackStatus=="approve_successful" ) {
        //alert("well done");
        $("#user_uid-" + uid).hide('slow');
        $("#user_uid-" + uid).next().hide('slow');
        location.reload();
      }
      // lock was not removed, display error
      else {
    	  alert("Ajax error, please refresh the page try again!");
        location.reload();
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      // remove spinner class on accordion row to show AJAX running
      //$("#" + accordionId).removeClass("ui-state-ajax");
  	  alert("Ajax error, please refresh the page try again!");
      location.reload();
    }
  });
  return false;
}

/**
 * This function dispalys reason to ban form in lightbox
 * @param {Object} uid
 */
function banUser(uid) {
  var ban_form = '#ban_form-' + uid;
  $.fn.colorbox({
    width: '40%',
    inline:true,
    href: ban_form,
  });
  return false;
}

/**
 * Process user delete when moderator clicks on the confirmation button
 * @param {Object} uid
 */

function confirmBanUser(uid){
  var ban_form = "#ban_form-" + uid;
  var tt =  $(ban_form + " input:radio:checked").length;
   
  if(tt > 0) {

    var reason_id = $(ban_form + " input:radio:checked").val();
    var ajax_url = "/admin/moderation/user-role/action/ban/" + uid + "/" + reason_id;

    $.ajax({
      url: ajax_url ,
      type: "GET",
      dataType: "text",
      timeout: 10000,
      success: function (data, textStatus, XMLHttpRequest) {
  	    // remove spinner class on accordion row to show AJAX running
  	    // $("#" + accordionId).removeClass("ui-state-ajax");
        
        // was callback successful?
        var callbackUid = data.split("&")[0].split("=").pop();
        var callbackStatus = data.split("&")[1].split("=").pop();
        
        //Expected data uid=valid&status=ban_successful
        if (callbackUid == "valid" && callbackStatus=="ban_successful" ) {
          $("#user_uid-" + uid).hide('slow');
          $("#user_uid-" + uid).next().hide('slow');
          location.reload();
        }
        // lock was not removed, display error 
        else {
        	  alert("Ajax error, please refresh the page try again!");
            location.reload();
        }
        
        
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        // remove spinner class on accordion row to show AJAX running
        //$("#" + accordionId).removeClass("ui-state-ajax");
        
    	  alert("Ajax error, please refresh the page try again!");
        location.reload();
      }
    });

  } else {
    alert("Please select a reason.");
    return false;  
  }
  return false;
}
