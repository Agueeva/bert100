  var myElement=new Array();
  var n=0;
  var myVarPattern= new Array("pat_gen_sel","prbs_gen_inv","prbs_autovr","pat_ver_sel"); //,"Loopback_en","tx_disable","pat_ver_en","pat_gen_en","error_insert"
  var myVarTX= new Array("txa_swing");
  var myVarDrTr= new Array("synth0.freq");
  var my_Interval, bl_Communication, all;
  var socket;
  var urlWS='ws://tneuner.homeip.net:8080/messages';  //'ws://' + document.domain + ':' + document.location.port + '/messages';
     //alert(urlWS);
     bl_Communication=false;
   function SocketNew() {
     delete socket;
     socket = new WebSocket(urlWS);
     
 socket = new WebSocket(urlWS);
 socket.onopen = function() {
             alert("Verbindung open");
             bl_Communication=true;
	     my_Interval=setInterval(keepAlive,30000);

     }
     socket.onclose = function() {
		alert('Verbindung unterbrochen');
		bl_Communication=false;
                my_Interval=clearInterval();
     }
     socket.onmessage = function(evt) {
	var arr = JSON.parse(evt.data);
	var cnt = 0;
	var item =arr['var'];
	var value =arr['val'];

        var var_id=$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr('id');

        if(typeof var_id == "undefined") {
         var_id=$("#frame").contents().find("#var_val").val(value);

         }
         else{
          $("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);
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
           $("#content div").show();

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
                                            $("#content div").fadeOut(200);
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
/**---------------------TreeMenu---------------------------
/**
* This function creates a simple tree menu out of a "ul" list.
* Icons can be defined via a css file.
*
* The syntax in your html should look like this:
* <ul id="treemenuID" class="menu">
*	   <li><a href='javascript:void(0);'>Item 1</a></li>
*	   <li>
*                     <a href='javascript:void(0);'>Folder 2</a>
*		      <ul>
*                                <li><a href='javascript:void(0);'>Item 2.1</a></li>
*                                <li><a href='javascript:void(0);'>Folder 2.2</a>
*                                            <ul>
*                                                       <li><a href='javascript:void(0);'>Item 2.2.1</a></li>
*                                            </ul>
*				 </li>
*				 <li><a href='javascript:void(0);'>Item 2.3</a></li>
*		      </ul>
*          </li>
* </ul>
*
* @param {Object} id    The name of the "ul" element that should be used for the tree menu.
* @return {}   Returns nothing.

*/
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
                all=false;
		laodpage("html/pattern.html","#frame");
		return false;
	});
	$( "#TXBut" ).click(function() {
	        myElement=myVarTX;
		n=1;
		laodpage("html/tx.html","#frame");
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
  var j,i;
  switch(n)
{
case 1:
  for (j = 0; j < myElement.length; j++){
  for (i = 0; i <= 3; i++){
  my_var="cdr0.l"+i+"."+myElement[j];
 
  socket.send(JSON.stringify({get: my_var}));
  }}
  break;
case 2:
  for (j = 0; j < myElement.length; j++){
  my_var=myElement[j];
  socket.send(JSON.stringify({get: my_var}));
//  alert(my_var);
}
  break;
default:
break;
}

}



function myCheckAll(box)
{
var j,i,item;
if (box.checked){
all=true;
for (j = 0; j < myElement.length; j++){
for (i = 1; i <= 3; i++){
item="cdr0.l"+i+"."+myElement[j];
	$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
}}
}
else {
all=false;
for (j = 0; j < myElement.length; j++){
for (i = 1; i <= 3; i++){
item="cdr0.l"+i+"."+myElement[j];
	$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",false);
}}
}
}

function SaveVar(myVar, typeVar){
if(!bl_Communication) SocketNew();
var formval, myID, myForm, myBool;
myID=myVar.id;
formval=myVar.value;
 switch(typeVar)
{
case 1:  //einfach
     
socket.send(JSON.stringify({set: myID,val: formval}));
  break;
case 2:  //all vorhandeln

if(all) {
for (i = 0; i <= 3; i++) {
socket.send(JSON.stringify({set: myID,val: formval}));
myID=myID.replace(".l"+i, ".l"+(i+1));
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

