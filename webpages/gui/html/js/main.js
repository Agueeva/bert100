  var myElement=new Array();
  var n=0;
  var myVarPattern= new Array("pat_gen_sel","prbs_gen_inv","prbs_autovr","pat_ver_sel"); //,"Loopback_en","tx_disable","pat_ver_en","pat_gen_en","error_insert"
  var myVarTX= new Array("txa_swing");
  var myVarDrTr= new Array("synth0.freq");
  var urlWS='ws://' + document.domain + ':' + document.location.port + '/messages';
     var socket = new WebSocket(urlWS);
     socket.onopen = function() {
		//alert('Verbindung aufgebaut');

     };
     socket.onclose = function() {
		alert('Verbindung unterbrochen');
     };
     socket.onmessage = function(evt) {
	var arr = JSON.parse(evt.data);
	var cnt = 0;
	var item =arr['var'];
	var value =arr['val'];
	if(typeof arr['var'] != "undefined") {
		$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).val(value);
	}

     }



//---------------------Laod page---------------------
var requestToRelaod = false;
var newUrl = "";
var pageLoading = false;
var timeout;
function laodpage(url,id) {
           if (pageLoading) {
                  requestToRelaod=true;
                  newUrl = url;
                  return;
           }
           $("#content div").fadeIn(200);
           $(id).stop().fadeOut(200);
           pageLoading = true;
           timeout = setTimeout(function(){
                      $(id).hide();
                      if (requestToRelaod) {
                                 pageLoading = false;
                                 requestToRelaod = false;
                                 clearTimeout (timeout);
                                 laodpage(newUrl,id);
                                 return;
                      }
		      $(id).attr("src",url);
                      $(id).load(function() {
                                 pageLoading = false;
                                 if (requestToRelaod) {
                                            requestToRelaod = false;
                                            clearTimeout (timeout);
                                            laodpage(newUrl,id);
                                            return;
                                 }else{
                                      $(id).fadeIn(200);
                                      $("#content div").fadeOut(200);
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
           $("#"+id+" li").each(function(index) {
                      //console.log(index);
                      var $$ = $(this);
                      $$.addClass("item");
                      $$.has("ul").removeClass("item").addClass("cat_close").addClass("category");
           });

           //hide all sub entries
           $("#"+id).find("ul").css("display", "none");

           $("#"+id+" .category > a").click(function() {
                      var $$ = $(this).parent();
		      var childid = $$.find("ul:first");
                      // open/close a entry
                      if ($(childid).css("display") == "none") {
                                 $(childid).fadeIn();
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
                      $("#treemenu .selected").removeClass("selected");
                      $(this).addClass("selected");
           });
           //$(id).hide();
}
//---------------------TreeMenu END-----------------------
$(document).ready(function()
{
           laodpage("html/main.html","#frame");

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

           createTreeMenu("treemenu");
});




function window_onload()
{
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
}
  break;
default:

}
  
}



function myCheckAll(box)
{
var j,i,item;
if (box.checked){

for (j = 0; j < myElement.length; j++){
for (i = 1; i <= 3; i++){
item="cdr0.l"+i+"."+myElement[j];
	$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",true);
}}
}
else {
for (j = 0; j < myElement.length; j++){
for (i = 1; i <= 3; i++){
item="cdr0.l"+i+"."+myElement[j];
	$("#frame").contents().find("#"+item.replace(/[.]/g,"\\.")).attr("disabled",false);
}}
}
}

function SaveVar(myVar){
var formval, myID, myForm, myBool;
myForm=	$("#frame").contents().find("#all");
myBool=myForm.checked;

myID=myVar.id;
formval=myVar.value;
if (formval=="checkbox") formval=myVar.checked?1:0;
if(myBool) {
for (i = 0; i <= 3; i++) {
socket.send(JSON.stringify({set: myID,val: formval}));
myID=myID.replace("l"+i, "l"+(i+1));
}
}
else {
socket.send(JSON.stringify({set: myID,val: formval}));

}
}

