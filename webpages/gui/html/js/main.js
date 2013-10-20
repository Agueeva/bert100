  var myElement=new Array();
  var myQueue=new Array();
  var myQueueBool=false;
  var myQueueCount=0;
  var n=0;
  var my_sek=250;
  var my_Interval, bl_Communication, all;
  var socket,page_k,page_pref, all_pat, all_tx;
  var prbs_autovr0=1,prbs_autovr1=1,prbs_autovr2=1,prbs_autovr3=1;
  var myVarPattern= new Array("bert0.L0.prbsPatGenSel","bert0.L1.prbsPatGenSel","bert0.L2.prbsPatGenSel","bert0.L3.prbsPatGenSel",
                              "bert0.L0.prbsVerInv","bert0.L1.prbsVerInv","bert0.L2.prbsVerInv","bert0.L3.prbsVerInv",
                              "bert0.L0.patVerSel","bert0.L1.patVerSel","bert0.L2.patVerSel","bert0.L3.patVerSel");  
  var myVarPattern0= new Array("prbsPatGenSel","prbsVerInv","patVerSel"); 
  var myVarTX= new Array("emlAmp1.vg1","emlAmp2.vg1","emlAmp3.vg1","emlAmp4.vg1",
			 "emlAmp1.vg2","emlAmp2.vg2","emlAmp3.vg2","emlAmp4.vg2",
			 "bert0.L0.txaSwing","bert0.L1.txaSwing","bert0.L2.txaSwing","bert0.L3.txaSwing",
			 "bert0.L0.swapTxPN","bert0.L1.swapTxPN","bert0.L2.swapTxPN","bert0.L3.swapTxPN");
  var myVarTX0= new Array("vg1","vg2");
  var myVarTX2= new Array("swapTxPN","txaSwing");
  var myVarDrTr= new Array("bert0.bitrate","ptrig0.pattern");
  var myVarErr= new Array("bert0.bitrate",
                          "bert0.L0.patVerSel","bert0.L1.patVerSel","bert0.L2.patVerSel","bert0.L3.patVerSel",
                          "bert0.L0.EqState","bert0.L1.EqState","bert0.L2.EqState","bert0.L3.EqState",
                          "bert0.L0.latchedLol","bert0.L1.latchedLol","bert0.L2.latchedLol","bert0.L3.latchedLol",
                          "bert0.L0.prbsLock","bert0.L1.prbsLock","bert0.L2.prbsLock","bert0.L3.prbsLock",
                          "bert0.L0.accErrCntr","bert0.L1.accErrCntr","bert0.L2.accErrCntr","bert0.L3.accErrCntr",
                          "bert0.L0.LolStat","bert0.L1.LolStat","bert0.L2.LolStat","bert0.L3.LolStat",
                          "bert0.L0.currBeRatio","bert0.L1.currBeRatio","bert0.L2.currBeRatio","bert0.L3.currBeRatio",
                          "bert0.rxPllLock","bert0.txPllLock",
                          "bert0.L0.accBeRatio", "bert0.L1.accBeRatio","bert0.L2.accBeRatio","bert0.L3.accBeRatio",
                          "bert0.L0.absErrCntr","bert0.L1.absErrCntr","bert0.L2.absErrCntr","bert0.L3.absErrCntr",
                          "bert0.L0.accTime",
                          "bert0.L0.CdrTrip" ,"bert0.L1.CdrTrip" ,"bert0.L2.CdrTrip" ,"bert0.L3.CdrTrip" ); 
  
  var myVarSystem= new Array("fanco.fan0.rpm","fanco.fan1.rpm","fanco.fan2.rpm",
                             "system.firmware","system.ip","system.netmask","system.mac","system.gateway","system.temp"); 
 
  var urlWS=  'ws://' + document.domain + ':' + document.location.port + '/messages'; // 'ws://tneuner.homeip.net:8080/messages'; //
     
     bl_Communication=true;
     all_pat=false;
     all_tx=false;
     
/* Socket-Functions */

   function SocketNew()
{
     if (typeof(socket) != "undefined") {delete socket;}
     socket = new WebSocket(urlWS);

 socket.onopen = function() {
             //alert("Verbindung open");
             bl_Communication=true;
	     my_Interval=setInterval(keepAlive,3000);
}
     socket.onclose = function()
     {
		//alert('Verbindung unterbrochen');
		bl_Communication=false;
                my_Interval=clearInterval(my_Interval);
}
     socket.onmessage = function(evt)
     {
        var myCh;
	var arr = JSON.parse(evt.data);
	var cnt = 0;
	var item =arr['var'];
	var value =arr['val'];
    if (item.substr(0, 6)=="emlAmp") {
          value=Math.round(value * 100) / 100;
          }
       if (item.substr(9, 7)=="CdrTrip") {
         myCh="#Loss"+item.substr(7, 1);
          if ( $("#frame").contents().find(myCh).attr('class')=='redfield') {
               value="--";
          }
          }
          
          
      if (item.substr(9, 7)=="EqState") {      
          $("#frame").contents().find(("#"+item+"_st").replace(/[.]/g,"\\.")).width((100-value*7)+"%");
          }
      if (item.substr(9, 11)=="currBeRatio" ||  item.substr(9, 10)=="accBeRatio") {
          myCh="#Loss"+item.substr(7, 1);
          if ( $("#frame").contents().find(myCh).attr('class')=='redfield') {
               value="--";
          }
          else{
          value=value.toExponential();
          var my_str=value.toString();
          if (my_str.match('e')) {
          var my_arr=my_str.split("e");
          var erst=Math.round(Number(my_arr[0]) * 100) / 100;
          my_str=erst.toString();
          my_str=my_str.concat("E");
          value=my_str.concat(my_arr[1]);
         } }         
          }
         // bert0.L0.absErrCntr
      if (item.substr(9, 10)=="absErrCntr" ||  item.substr(9, 10)=="accErrCntr" ) {
          myCh="#Loss"+item.substr(7, 1);
          if ( $("#frame").contents().find(myCh).attr('class')=='redfield') {
               value="--";
          }
          else{
          value=value.toExponential();
          var my_str=value.toString();
          if (my_str.match('e')) {
          var my_arr=my_str.split("e");
          var erst=Number(my_arr[0]);
          my_str=erst.toString();
          my_str=my_str.concat("E");
          value=my_str.concat(my_arr[1]);
          }         
          }}

      switch(item)
     {
     case "test.var1":
       document.getElementById('test.var1').value=value;
     return;
     
     // "bert0.L0.accTime
      case "bert0.L0.accTime":
          //alert(value);
     if (value!=0) {
       /*   var wert=Number(value);
          var    my_Tag = Math.floor(wert/86400);
          var d = new Date(wert*1000);
         
     value = '%02d:%02d:%02d:%02d'.sprintf(my_Tag,d.getHours(), d.getMinutes(), d.getSeconds());
     alert(value);*/
          var wert=Number(value);
      var    my_Tag = Math.floor(wert/86400);
      wert=wert-my_Tag*86400;
      var    my_Stunde= Math.floor(wert/3600);
      wert=wert-my_Stunde*3600;
      var    my_Minuten=Math.floor(wert/60);
      var    my_Sek=Math.floor(wert-my_Minuten*60);
      value= my_Tag+ ":" + my_Stunde + ":" + my_Minuten + ":" + my_Sek;
          } 
     break;
     
     
     case "system.temp":
     if (value!=0) {
      value=Math.round(Number(value) * 10)/ 10;
          } 
     break;
     
     case "bert0.L0.LolStat":
     if (value==0) {
      $("#frame").contents().find("#Loss0").attr('class','greenfield');
          }else{
      $("#frame").contents().find("#Loss0").attr('class','redfield');
          } 
     break;

     case "bert0.L1.LolStat":
     if (value==0) {
      $("#frame").contents().find("#Loss1").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Loss1").attr('class','redfield');
     }
     break;

     case "bert0.L2.LolStat":
     if (value==0) {
      $("#frame").contents().find("#Loss2").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Loss2").attr('class','redfield');
     }
     break;
     
     case "bert0.L3.LolStat":
      if (value==0) {
      $("#frame").contents().find("#Loss3").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Loss3").attr('class','redfield');
     }
     break;

     case "bert0.L0.latchedLol":
     if (value==0) {
      $("#frame").contents().find("#Mem0").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Mem0").attr('class','redfield');
     }
     break;

     case "bert0.L1.latchedLol":
     if (value==0) {
      $("#frame").contents().find("#Mem1").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Mem1").attr('class','redfield');
     }
     break;

     case "bert0.L2.latchedLol":
      if (value==0) {
      $("#frame").contents().find("#Mem2").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Mem2").attr('class','redfield');
     }
     break;

     case "bert0.L3.latchedLol":
      if (value==0) {
      $("#frame").contents().find("#Mem3").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Mem3").attr('class','redfield');
     }
     break;

     case "bert0.L0.prbsLock":
     if (value==1) {
      $("#frame").contents().find("#Lock0").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Lock0").attr('class','redfield');
     }
     break;

     case "bert0.L1.prbsLock":
     if (value==1) {
      $("#frame").contents().find("#Lock1").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Lock1").attr('class','redfield');
     }
     break;

     case "bert0.L2.prbsLock":
      if (value==1) {
      $("#frame").contents().find("#Lock2").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Lock2").attr('class','redfield');
     }
     break;

     case "bert0.L3.prbsLock":
      if (value==1) {
      $("#frame").contents().find("#Lock3").attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Lock3").attr('class','redfield');
     }
     break;

     case "bert0.txPllLock":
     if (value==0) {
      $("#frame").contents().find("#TX").attr('class','redfield');
     } else {
      $("#frame").contents().find("#TX").attr('class','greenfield');
     }
      break;
     
     case "bert0.rxPllLock":
     if (value==0) {
      $("#frame").contents().find("#RX").attr('class','redfield');
     } else {
      $("#frame").contents().find("#RX").attr('class','greenfield');
     }
     break;
     
     case "bert0.berMeasWin_ms":    
      $("#frame").contents().find("#msec").val(value/1000);     
     break;
     
     case "bert0.bitrate":
     if (value>25781249900 && value<25781250080) {
         value=25781250000;}
     if (value>27952493300 && value<27952493482) {
         value=27952493392;}
     break; 
     // 27952493320
     default:
     break;
     }
    
     var var_id=$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr('id');
     var variab=$("#frame").contents().find("#var_val").attr('id');
          if(typeof var_id == "undefined" && typeof variab != "undefined") {
               var_id=$("#frame").contents().find("#var_val").val(value);}
          else{
               $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);}
          if (myQueueBool) {  
               window_onload_variable();}      
}
   

     function keepAlive() {
          socket.send(JSON.stringify({get: "test.var1"}));
} 
}
   
/* Socket-Functions End */ 
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
                
        	// myElement=myVarSystem;
                //n=2;
		//laodpage("html/system.html","#frame");
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
		n=2;
                all=all_pat;
                page_pref="bert0.L";
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
	$( "#SystemBut" ).click(function() {
		myElement=myVarSystem;
                n=2;
		laodpage("html/system.html","#frame");
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
  myQueue=[];
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
	if (myQueue.length>0) {
	socket.send(JSON.stringify({get: myQueue[0]}));  
	}
        
	break;
      case 2:
        myQueue=myElement;
        myQueueBool=true;
        myQueueCount=1;
        if (myQueue.length>0) {
	socket.send(JSON.stringify({get: myQueue[0]})); 
	}
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
        //alert(my_var.id);
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
     item='l'+i+'.prbs_autovr';
     if ($("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val()==1) {
          item='bert0.L'+i+'.patVerSel'; 
          $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
     }
    }
    
   
}

function SaveVar(myVar, typeVar,pref,k){
    if(!bl_Communication) SocketNew();
          var formval, myID, myForm, myBool;     
          myID=myVar.id;
          formval=myVar.value;
          
     switch(typeVar){
	case 1:  //einfach
	    // alert("SV1:" +myID+"="+formval);
	    socket.send(JSON.stringify({set: myID,val: formval}));
	    break;
	case 2:  //all vorhandeln	
	    if(all) {
               for (i = 0; i <= 3; i++) {
                //   alert("SV:" +myID+"="+formval);
                    socket.send(JSON.stringify({set: myID,val: formval}));
                    myID=myID.replace(pref+(i+k), pref+(i+1+k));
                    }
               }else {
                    socket.send(JSON.stringify({set: myID,val: formval}));
               }
	  break;
          default:
	  break;
    }
}

