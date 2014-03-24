  var myElement=new Array();
  var myQueue=new Array();
  var myQueueBool=false;
  var myQueueCount=0;
  var n=0;
  var my_sek=300;
  var my_Interval, bl_Communication, all;
  var socket,page_k,page_pref, all_pat, all_tx, page_act="Index";
  var prbs_autovr0=1,prbs_autovr1=1,prbs_autovr2=1,prbs_autovr3=1;
  var dataset=-1;
  Alarm_Strings=new Array("Mod.Overtemp","Amp. Overtemp","CPU (na)","FAN1 fault","FAN2 fault","FAN3 fault","FAN4 fault","Alarm");

  var myVarPattern= new Array("bert0.L0.patGenSel","bert0.L1.patGenSel","bert0.L2.patGenSel","bert0.L3.patGenSel",
                              "bert0.L0.prbsVerInv","bert0.L1.prbsVerInv","bert0.L2.prbsVerInv","bert0.L3.prbsVerInv",
                              "bert0.L0.patVerSel","bert0.L1.patVerSel","bert0.L2.patVerSel","bert0.L3.patVerSel");  
  var myVarPattern0= new Array("patGenSel","prbsVerInv","patVerSel");
  var myVarTX_opt_test=new Array("bert0.L0.txaSwingFine", "bert0.L1.txaSwingFine", "bert0.L2.txaSwingFine", "bert0.L3.txaSwingFine",
                         "bert0.L0.txaSwing","bert0.L1.txaSwing","bert0.L2.txaSwing","bert0.L3.txaSwing",
                         "bert0.L0.txaEqpst","bert0.L1.txaEqpst","bert0.L2.txaEqpst","bert0.L3.txaEqpst",
                         "bert0.L0.txaEqpre","bert0.L1.txaEqpre","bert0.L2.txaEqpre","bert0.L3.txaEqpre",
			 "bert0.L0.swapTxPN","bert0.L1.swapTxPN","bert0.L2.swapTxPN","bert0.L3.swapTxPN",
                         "amp1.vg1","amp2.vg1","amp3.vg1","amp0.vg1",
                         "amp1.vg2","amp2.vg2","amp3.vg2","amp0.vg2",
			 "amp1.vd1","amp2.vd1","amp3.vd1","amp0.vd1",
                         "mzMod0.Ki","mzMod1.Ki","mzMod2.Ki","mzMod3.Ki",
                         "amp1.vd2","amp2.vd2","amp3.vd2","amp0.vd2","bert0.dataSetDescription","mzMod.shiftEyeSym");
  var myVarTX_opt_test0= new Array("vg1","vg2","vd1","vd2");
  var myVarTX_opt_test2= new Array("txaSwingFine","swapTxPN","txaSwing","swapTxPN");
  
  var myVarTX_opt= new Array("mzMod0.modBias","mzMod1.modBias","mzMod2.modBias","mzMod3.modBias",
                             "mzMod1.ctrlDev","mzMod2.ctrlDev","mzMod3.ctrlDev","mzMod0.ctrlDev",
                             "bert0.L0.swapTxPN","bert0.L1.swapTxPN","bert0.L2.swapTxPN","bert0.L3.swapTxPN",
                             "mzMod0.ctrlEnable","mzMod1.ctrlEnable","mzMod2.ctrlEnable","mzMod3.ctrlEnable",
			     "tx0.pwr","tx1.pwr","tx2.pwr","tx3.pwr",
                             "tx0.pwrRef","tx1.pwrRef","tx2.pwrRef","tx3.pwrRef",
                             "mzMod0.bias","mzMod1.bias","mzMod2.bias","mzMod3.bias","bert0.dataSetDescription");
  var myVarTX_opt0= new Array("swapTxPN");
  var myVarTX_opt1= new Array("ctrlEnable");
  var myVarDrTr= new Array("ptrig0.pattern","bert0.bitrate");
  var myVarErr= new Array("mzMod0.modBias","mzMod1.modBias","mzMod2.modBias","mzMod3.modBias","bert0.bitrate",
                          "mzMod0.ctrlFault", "mzMod1.ctrlFault","mzMod2.ctrlFault","mzMod3.ctrlFault",
                          "mzMod0.latchedCtrlFault", "mzMod1.latchedCtrlFault", "mzMod2.latchedCtrlFault", "mzMod3.latchedCtrlFault",
                          "mzMod0.ctrlDev","mzMod1.ctrlDev","mzMod2.ctrlDev","mzMod3.ctrlDev",
                          "tx0.pwr","tx1.pwr","tx2.pwr","tx3.pwr",
                          "mzMod0.ctrlEnable","mzMod1.ctrlEnable","mzMod2.ctrlEnable","mzMod3.ctrlEnable",
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
                          "bert0.L0.CdrTrip" ,"bert0.L1.CdrTrip" ,"bert0.L2.CdrTrip" ,"bert0.L3.CdrTrip"
                         ); 
  
  var myVarSystem= new Array("fanco.fan0.rpm","fanco.fan1.rpm","fanco.fan2.rpm","fanco.fan3.rpm","system.latchedFault",
                             "system.firmware","system.ip","system.netmask","system.mac","system.gateway","system.temp","amp.temp","mzMod.temp","system.variant","system.hwRevision");
  var myVarGraph= new Array("mzMod0.modBias","mzMod1.modBias","mzMod2.modBias","mzMod3.modBias",
			 "tx0.pwr","tx1.pwr","tx2.pwr","tx3.pwr");
 
  var urlWS= 'ws://' + document.domain + ':' + document.location.port + '/messages'; // 
     
     bl_Communication=true;
     all_pat=false;
     all_tx=false;
     all_tx_opt_test=false;
/* Socket-Functions */

   function SocketNew()
{
     if (typeof(socket) != "undefined") {delete socket;}
     socket = new WebSocket(urlWS);

 socket.onopen = function() {
             bl_Communication=true;
	     my_Interval=setInterval(keepAlive,3000);
}
     socket.onclose = function(){
		bl_Communication=false;
                my_Interval=clearInterval(my_Interval);
}
     socket.onmessage = function(evt){
        var myCh;
        var mystr;
      
   if (evt.data.substr(2, 3)=="ack") { return;}
	var arr = JSON.parse(evt.data);
	var cnt = 0;
	var item =arr['var'];
	var value =arr['val'];
       console.log("item:"+item+" Value:"+value);
        if (item.substr(9, item.length)=="patGenSel" && value==3) {
         ReadVarByName("bert0.userPattern");
          $("#frame").contents().find("#userPattern0").css('display','table-row');
           mystr='l'+item.substr(7, 1)+'.prbs_autovr';
           $("#frame").contents().find("#"+mystr.replace(/[.]/g,"\\.")).attr("disabled",true);
           mystr=item.substr(0, 8)+'.patVerSel';
           $("#frame").contents().find("#"+mystr.replace(/[.]/g,"\\.")).attr("disabled",true);
          }
    if ((item.substr(0, 3)=="amp" && item!="amp.temp") || (item.substr(0, 5)=="mzMod" && item!="mzMod.temp" && item!="mzMod.shiftEyeSym") || item.substr(4, 3)=="pwr") {
     
          value=Math.round(value * 100) / 100;
          if (value>10) {
           value='>10';    
          }
           if (value<-10) {
           value='<-10';    
          }
          }
       if (item.substr(9, 7)=="CdrTrip") {  //bert0.L0.CdrTrip
          
         myCh="#Loss"+item.substr(7, 1);
          if ( $("#frame").contents().find(myCh).attr('class')=='redfield') {
               value="--";
               $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', 50+"%");
          }else {
               if (value < -200){
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', 2+"%");
               }else if( value > 200) {
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', 99+"%");
               }else{
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', ((value+200)/4)+"%");
               }
          }
          }
         if (item.substr(7, 7)=="ctrlDev") {  //mzMod0.ctrlDev
         
               if (value < -0.5){
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', 2+"%");
               }else if( value > 0.5) {
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', 99+"%");
               }else{
                    $("#frame").contents().find(("#"+item+"_line").replace(/[.]/g,"\\.")).css('left', (value*100+50)+"%");
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
var k;
 //**********neu if aus case************************************         
     if (item.substr(9, 7)=="LolStat") {
          k=item.substr(7, 1);
       if (value==0) {
      $("#frame").contents().find("#Loss"+k).attr('class','greenfield');
          }else{
      $("#frame").contents().find("#Loss"+k).attr('class','redfield');
          }
     }
     if (item.substr(9, 10)=="latchedLol") {
           k=item.substr(7, 1);
       if (value==0) {
      $("#frame").contents().find("#Mem"+k).attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Mem"+k).attr('class','redfield');
     }
     }
     if (item.substr(9, 8)=="prbsLock") {
          k=item.substr(7, 1);
       if (value==1) {
      $("#frame").contents().find("#Lock"+k).attr('class','greenfield');
     }else{
      $("#frame").contents().find("#Lock"+k).attr('class','redfield');
     }
     }
     
  //************************************************************************  "mzMod0.ctrlFault" mzMod0.ctrlEnable  mzMod0.latchedCtrlFault
     if (item.substr(7, 9)=="ctrlFault") {
          k=item.substr(5, 1);
       if (value==0) {
      $("#frame").contents().find("#Fault"+k).attr('class','greenfield');
          }else{
      $("#frame").contents().find("#Fault"+k).attr('class','redfield');
          }
     }
     if (item.substr(7, 16)=="latchedCtrlFault") {
           k=item.substr(5, 1);          
       if (value==0) {
      $("#frame").contents().find("#MZMem"+k).attr('class','greenfield');
     }else{
      $("#frame").contents().find("#MZMem"+k).attr('class','redfield');
     }
     }  
     
 //***********************************************************************+  "mzMod.shiftEyeSym"
 if (page_act=="Measure" && item.substr(9, 9)=="patVerSel") {
var item_c=item.substr(0, 9)+"patVerSel_Text";
 $("#frame").contents().find("#"+item_c.replace(/[.]/g,"\\.")).val(value);
 }
switch(item)
     {
case "test.var1":
       document.getElementById('test.var1').value=value;
     return;
case "mzMod.shiftEyeSym":
      value= Math.round(value); 
     break;
case "system.latchedFault":
     
      var meldung="";
       if (value!=0) {
         var fehler=value.toString(2);
         for (i=fehler.length-1; i>=0; i--){
          if (fehler.charAt(i)==1) {
          meldung=meldung+Alarm_Strings[fehler.length-1-i]+" ";}
         }
       }
       $("#frame").contents().find("#FaultDescription").val(meldung);
     break;
case "system.fault":
       document.getElementById(item).value=value;
       if (value==0) {
         Alarmindicator.className = "AlarmindicatorGreen";
             }
       else {
         var fehler=value.toString(2);
         var meldung="System failing: ";
         for (i=fehler.length-1; i>=0; i--){
          if (fehler.charAt(i)==1) {
          meldung=meldung+Alarm_Strings[fehler.length-1-i]+"; ";}
         }
          $("#Alarmindicator").html(meldung);   
          Alarmindicator.className = "AlarmindicatorRed";
          document.getElementsById('latchedFault').style.display='table-row';	 
       }
     return;
case  "system.variant":
      value=value+" TX";
     break; 
case "bert0.L0.accTime":
     if (value!=0) {
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
case "bert0.userPattern":
      var i;
      var valuebinary=parseInt(value, 16).toString(2);
      for (i = 1; i <= 4; i++){
          item="userHex"+i;
          value=valuebinary.substr((i-1)*10,10);
          $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);}
     return;

case "system.temp":
     if (value!=0) {
      value=Math.round(Number(value) * 10)/ 10;
          } 
     break;
case "mzMod.temp":
     if (value!=0) {
      value=Math.round(Number(value) * 10)/ 10;
          } 
     break;
case "amp.temp":
     if (value!=0) {
      value=Math.round(Number(value) * 10)/ 10;
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
         $("#frame").contents().find("#userDR").val((Math.round(Number(value/100))/10000000));
     if ( value!=25781250000 &&  value!=27952493392){
          $("#frame").contents().find("#bert0.bitrate".replace(/[.]/g,"\\.")).val(-1);
          $("#frame").contents().find('#noneUserDateRate').hide();
          $("#frame").contents().find('#userDateRate').show();
          return;     
     }else{
         $("#frame").contents().find('#noneUserDateRate').show();
          $("#frame").contents().find('#userDateRate').hide();
          //console.log("hide");
     }
     break; 
     // 27952493320
default:
     break;
     }
     
     var var_id=$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr('id');
     var variab=$("#frame").contents().find("#var_val").attr('id');
          if(typeof var_id == "undefined" && typeof variab != "undefined") {
               var_id=$("#frame").contents().find("#var_val").val(value);
               }else{
               $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);}
          if (myQueueBool) {  
               window_onload_variable();}      


}
   

     function keepAlive() {
          socket.send(JSON.stringify({get: "system.fault"}));
}

}
   
  
/* Socket-Functions End */ 
//---------------------Laod page---------------------
var requestToRelaod = false;
var newUrl = "";
var pageLoading = false;
var timeout;
function laodpage(url,id) {
           $(id).hide();
           $("#content_wrapper div").show();

           if (pageLoading) {
                      clearTimeout(timeout);
                      pageLoading = false;
                      requestToRelaod = true;
           }

           pageLoading = true;
           timeout = setTimeout(function(){
		      $(id).attr("src",url);
                      $(id).load(function() {
                                 if (!requestToRelaod) {
                                            $(id).fadeIn(200);
                                            $("#content_wrapper div").fadeOut(200);
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
                      }else{
                                 $(childid).hide();
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
                page_act="Index"
		laodpage("html/main.html","#frame");
		return false;
	});

	//---HOME BUTTON---
	//Click
	$( "#PatternBut" ).click(function() {
		myElement=myVarPattern0;
		n=1;
                all=all_pat;
                page_pref="bert0.L";
                page_k=0;
                page_act="Pattern"
		laodpage("html/pattern.html","#frame");
                
		return false;
	});
	

	$( "#RateTrBut" ).click(function() {
	        myElement=myVarDrTr;
		n=2;
                page_k=0;
                page_act="RateTrBut"
		laodpage("html/drtr.html","#frame");
		return false;
	});
      
        $( "#MeasureBut" ).click(function() {
		myElement=myVarErr;
                n=2;
                page_k=0;
                page_act="Measure"
		laodpage("html/error.html","#frame");
		return false;
	});
       
	$( "#SystemBut" ).click(function() {
		myElement=myVarSystem;
                n=2;
                page_k=0;
                page_act="System"
		laodpage("html/system.html","#frame");
		return false;
	});
        $( "#OpticalTXBut" ).click(function() {
		myElement=myVarTX_opt;
                n=2;
                page_k=0;
                all=all_tx;
                page_act="OpticalTX"
		laodpage("html/tx_opt.html","#frame");
		return false;
	});
        $( "#OpticalTXTestBut" ).click(function() {
	        myElement=myVarTX_opt_test;
		n=2;
		all=all_tx_opt_test;
                page_pref="amp";
                page_k=0;
                page_act="OpticalTXcalibr"
		laodpage("admin/tx_opt_calibr.html","#frame");
		return false;
	});
       $( "#OpticalGraphBut" ).click(function() {
	        myElement=myVarGraph;
		n=2;
		all=all_tx_opt_test;
                page_pref="amp";
                page_k=0;
                page_act="Graph"
		laodpage("admin/graph.html","#frame");
		return false;
	});
        $( "#VariableBut" ).click(function() {
	        myElement=myVarGraph;
		n=2;
		all=all_tx_opt_test;
                page_pref="amp";
                page_k=0;
		laodpage("html/variablen.html","#frame");
		return false;
	});
           createTreeMenu("treemenu");
     }
     );

function ReadVar(variable){	      
		if(!bl_Communication) SocketNew(); 
		var my_var;
		my_var=variable.value;
		socket.send(JSON.stringify({get: my_var}));
     }
function ReadVarByName(variable){
		if(!bl_Communication) SocketNew();  
		socket.send(JSON.stringify({get: variable}));
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
	    socket.send(JSON.stringify({set: myID,val: formval}));
	    break;
	case 2:  //all vorhandeln	
	    if(all) {
               for (i = 0; i <= 3; i++) {
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
function SystemFaultreset(){
socket.send(JSON.stringify({set: "system.latchedFault",val: 0}));     
}
