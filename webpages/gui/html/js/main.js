  var myElement=new Array();
  var myQueue=new Array();
  var myQueueBool=false;
  var myQueueCount=0;
  var n=0;
  var myVarPattern= new Array("pat_gen_sel","prbs_gen_inv","prbs_autovr","pat_ver_sel");  //,"Loopback_en","tx_disable","pat_ver_en","pat_gen_en","error_insert");
  var myVarTX= new Array("emlAmp1.vg1","emlAmp2.vg1","emlAmp3.vg1","emlAmp4.vg1","emlAmp1.vg2",
                         "emlAmp2.vg2","emlAmp3.vg2","emlAmp4.vg2","cdr0.l0.txa_swing","cdr0.l1.txa_swing",
                         "cdr0.l2.txa_swing","cdr0.l3.txa_swing","cdr0.l0.Swap_TXP_N","cdr0.l1.Swap_TXP_N",
                         "cdr0.l2.Swap_TXP_N","cdr0.l3.Swap_TXP_N");
  var myVarTX0= new Array("vg1","vg2");
  var myVarTX1= new Array("txa_swing","Swap_TXP_N");
  var myVarDrTr= new Array("synth0.freq");
  var myVarErr= new Array("synth0.freq","cdr0.l0.pat_gen_sel","cdr0.l1.pat_gen_sel","cdr0.l2.pat_gen_sel",
                          "cdr0.l3.pat_gen_sel","cdr0.l0.eq_state","cdr0.l1.eq_state","cdr0.l2.eq_state",
                          "cdr0.l3.eq_state","cdr0.l0.err_cntr","cdr0.l1.err_cntr","cdr0.l2.err_cntr",
                          "cdr0.l0.latched_lol","cdr0.l1.latched_lol","cdr0.l2.latched_lol","cdr0.l3.latched_lol",
                          "cdr0.l0.prbs_lock","cdr0.l1.prbs_lock","cdr0.l2.prbs_lock","cdr0.l3.prbs_lock",
                          "cdr0.l0.no_prbs_lck","cdr0.l1.no_prbs_lck","cdr0.l2.no_prbs_lck","cdr0.l3.no_prbs_lck",
                          "cdr0.l0.err_cntr64","cdr0.l1.err_cntr64","cdr0.l2.err_cntr64","cdr0.l3.err_cntr64");
  var my_Interval, bl_Communication, all;
  var socket,page_k,page_pref, all_pat, all_tx;
  var urlWS='ws://' + document.domain + ':' + document.location.port + '/messages'; //'ws://tneuner.homeip.net:8080/messages'; // 
     //alert(urlWS);
     bl_Communication=true;
     all_pat=false;
     all_tx=false;
   function SocketNew() {
     delete socket;
     socket = new WebSocket(urlWS);

 socket = new WebSocket(urlWS);
 socket.onopen = function() {
             //alert("Verbindung open");
             bl_Communication=true;
	     my_Interval=setInterval(keepAlive,3000);

     }
     socket.onclose = function() {
		alert('Verbindung unterbrochen');
		bl_Communication=false;
                my_Interval=clearInterval(my_Interval);
     }
     socket.onmessage = function(evt) {
	var arr = JSON.parse(evt.data);
	var cnt = 0;
	var item =arr['var'];
	var value =arr['val'];
      if (item.substr(0, 6)=="emlAmp") {
          value=Math.round(value * 100) / 100;
      }
      switch(item)
{
case "test.var1":
  document.getElementById(item).value=value;
  break;
case "synth0.freq":
  if (value>644531240 && value<644531260) {
    value=644531250;
  }
   if (value>698812325 && value<698812345) {
    value=698812335;
  }
  // $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);
  //break;
default:
     var var_id=$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr('id');
        if(typeof var_id == "undefined") {
         var_id=$("#frame").contents().find("#var_val").val(value);
         }
         else{
          $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);
         }
         if (myQueueBool) {
          
          
          window_onload_variable();
         }
break;
}  
        
     }
  function keepAlive() {
          socket.send(JSON.stringify({get: "test.var1"}));
     }
     }
//---------------------Laod page---------------------
var requestToRelaod = false;
var newUrl = "";
var pageLoading = false;
var timeout;
function laodpage(url,id) {
           //$(id).stop();
           //$("#content div").stop();
           $(id).hide();
           $("#content_wrapper div").show();

           if (pageLoading) {
                      clearTimeout(timeout);
                      pageLoading = false;
                      requestToRelaod = true;
                     //laodpage(url,id);
                    // return;
           }

           pageLoading = true;
           timeout = setTimeout(function(){
		      $(id).attr("src",url);
                      $(id).load(function() {
                                 if (!requestToRelaod) {
                                            $(id).fadeIn(200);
                                            $("#content_wrapper div").fadeOut(200);
                                            //timeout=setTimeout(function(){
                                            //           pageLoading = false;
                                           // },210);
                                 }else{
                                            requestToRelaod=false;

                                            window_onload();
                                 }
                       });
		      },210);
           return;

}

var selected = null;
function createTreeMenu(id) {
           // go over tree to setup classes
           $("#"+id).parent().addClass("category")
           $("#"+id+" li").each(function(index) {
                      //console.log(index);
                      var $$ = $(this);
                      $$.addClass("item");
                      $$.has("ul").removeClass("item").addClass("cat_close").addClass("category");
           });

           $("#"+id+" li:first").addClass("selected");

           //hide all sub entries
           $("#"+id).find("ul").hide();

           $("#"+id+" .category > a").click(function() {
                      var $$ = $(this).parent();
		      var childid = $$.find("ul:first");
                      // open/close a entry
                      if ($(childid).css("display") == "none") {
                                 $(childid).fadeIn(200);
                                // $(childid).css("display", "block");
                      }else{
                                 $(childid).hide();
                                // $(childid).css("display", "none");
                      }
                      // change the css class of the entry (changees the icon)
                      if ($$.hasClass("cat_close")) {
                                 $$.removeClass("cat_close").addClass("cat_open");
                      }else{
                                 $$.removeClass("cat_open").addClass("cat_close");
                      }
		});

           $("#"+id+" .item > a").click(function() {
                      $("#"+id+" .selected").removeClass("selected").addClass("item");
                      $(this).parent().removeClass("item");
                      $(this).parent().addClass("selected");
           });
}
//---------------------TreeMenu END-----------------------
$(document).ready(function()
{
        	n=3;
          laodpage("html/main.html","#frame");
         SocketNew();
	//Click.
	$( "#homeBut" ).click(function() {

		laodpage("html/main.html","#frame");
		return false;
	});

	//---HOME BUTTON---
	//Click
	$( "#PatternBut" ).click(function() {
		myElement=myVarPattern;
		n=1;
                all=all_pat;
                page_pref="cdr0.l";
                page_k=0;
		laodpage("html/pattern.html","#frame");
		return false;
	});
	$( "#TXBut" ).click(function() {
	        myElement=myVarTX;
		n=2;
		all=all_tx;
                page_pref="emlAmp";
                page_k=1;
		laodpage("html/tx_eml.html","#frame");
		return false;
	});

	$( "#RateTrBut" ).click(function() {
	        myElement=myVarDrTr;
		n=2;
		laodpage("html/drtr.html","#frame");
		return false;
	});
        $( "#Variables" ).click(function() {
		n=3;
		laodpage("html/variablen.html","#frame");
		return false;
	});
        $( "#VariablesDAC0" ).click(function() {
		n=3;
		laodpage("html/variablendac0.html","#frame");
		return false;
	});
        $( "#VariablesADC12" ).click(function() {
		n=3;
		laodpage("html/variablenadc12.html","#frame");
		return false;
	});
        $( "#MeasureBut" ).click(function() {
		myElement=myVarErr;
                n=2;
		laodpage("html/error.html","#frame");
		return false;
	});

           createTreeMenu("treemenu");
     });

function ReadVar(variable){
	      //  alert(bl_Communication);
		if(!bl_Communication) SocketNew();
		var my_var;
		my_var=variable.value;
		socket.send(JSON.stringify({get: my_var}));
     }


function window_onload()
{
if(!bl_Communication) SocketNew();
  var my_var;
  var j,i,k;
  k=0;
  switch(n)
    {
      case 1:
	for (j = 0; j < myElement.length; j++){
	for (i = 0; i <= 3; i++){
	my_var=page_pref+(i+page_k)+"."+myElement[j];
         myQueue[k]=my_var;
         k++;
	}}
        myQueueBool=true;
        myQueueCount=1;
        socket.send(JSON.stringify({get: myQueue[0]}));
	break;
      case 2:
        myQueue=myElement;
        myQueueBool=true;
        myQueueCount=1;
        socket.send(JSON.stringify({get: myQueue[0]}));
	break;
      default:
        break;
    }
}

function window_onload_variable()
{
   
    // setTimeout(callback, 500);
     if (myQueueCount<myQueue.length) {     
  socket.send(JSON.stringify({get: myQueue[myQueueCount]}));
  myQueueCount++;
     }
     else {myQueueBool=false;
     myDisableAuto();}
}

function myCheckAll(box,prefB,pref,k)
{
    var j,i,item, my_var;
    if (box.checked){
      all=true;
      for (j = 0; j < myElement.length; j++){
	item =prefB+k+"."+myElement[j];
	var iframe = document.getElementById('frame');
	var frameDoc = iframe.contentDocument || iframe.contentWindow.document;
	my_var = frameDoc.getElementById(item);
	SaveVar(my_var, 2,pref,k);
	for (i = 1; i <= 3; i++){
	  item=prefB + (i+k) + "." + myElement[j];
	  $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
      }}
      }
      else {
	all=false;
	for (j = 0; j < myElement.length; j++){
	  for (i = 1; i <= 3; i++){
	    item=prefB + (i+k) + "." + myElement[j];
	    $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",false);
	}}
    }
   
}

function myDisableAll(box,prefB,pref,k)
{
    var j,i,item, my_var;
    if (box.checked){    
    for (j = 0; j < myElement.length; j++){
      item =prefB+k+"."+myElement[j];
    for (i = 1; i <= 3; i++){
      item=prefB + (i+k) + "." + myElement[j];
      $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
    }}
    
    }
    else {
    all=false;
    for (j = 0; j < myElement.length; j++){
      for (i = 1; i <= 3; i++){
	item=prefB + (i+k) + "." + myElement[j];
	$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",false);
      }}
    }
}
function myDisableAuto()
{
  
    for (i = 0; i < 4; i++){
      item='cdr0.l'+i+'.prbs_autovr';
      if ($("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val()==1) {
         item='cdr0.l'+i+'.pat_ver_sel'; 
        $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
      }
      
    }
    
   
}

function SaveVar(myVar, typeVar,pref,k){
    if(!bl_Communication) SocketNew();
      var formval, myID, myForm, myBool;     
      myID=myVar.id;
      formval=myVar.value;
      switch(typeVar)
    {
	case 1:  //einfach
	    //alert(myID+"="+formval);
	    socket.send(JSON.stringify({set: myID,val: formval}));
	    break;
	case 2:  //all vorhandeln	
	    if(all) {
	    for (i = 0; i <= 3; i++) {
	    socket.send(JSON.stringify({set: myID,val: formval}));
	    myID=myID.replace(pref+(i+k), pref+(i+1+k));
	    }
	    }
	    else {
	    socket.send(JSON.stringify({set: myID,val: formval}));
	    }
	  break;
	default:
	    break;
    }
}

