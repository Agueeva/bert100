
function ClickUp(i,ud,sr,k, bez, gr1,gr2){ 
    var my_item=document.getElementById('amp'+i+bez);
    switch(ud){
        case 1:
            if((Number(my_item.value)+sr*0.01) < gr1) {
               my_item.value=Math.round((Number(my_item.value)+(sr*0.01))* 100) / 100;
            }else {
             my_item.value=gr1;
            }
        break;
        case 2:
            if((my_item.value-sr*0.01) > gr2) {
                my_item.value=Math.round((Number(my_item.value)-(sr*0.01))* 100) / 100;
            }else{
                my_item.value=gr2;
            }
        break;
        default:
        break;
    }
TestVal(i,bez, gr1,gr2);

my_item=document.getElementById('amp'+i+bez);
window.parent.SaveVar(my_item,k,'amp',0);
if (k==2) {
  window.parent.myElement=window.parent.myVarTX_opt_test;
  window.parent.window_onload();
}
}
//emlAmp1.vg1
function TestVal(i, bez, gr1,gr2){
	var my_item=document.getElementById('amp'+i+bez);
	if (isNaN(my_item.value)) {
		
	}else {
		
		if(my_item.value < gr2) { my_item.value=gr2;}
		if(my_item.value > gr1) { my_item.value=gr1;}
                
	}
}


function Check_all() {
    var my_all=document.getElementById('all');
    if (window.parent.all_tx_opt_test) {
	my_all.checked=true;
	window.parent.myElement=window.parent.myVarTX_opt_test0;
	window.parent.myDisableAll(my_all,'amp','amp',0);
	window.parent.myElement=window.parent.myVarTX_opt_test2;
	window.parent.myDisableAll(my_all,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarTX_opt_test;
    }
    window.parent.socket.send(JSON.stringify({get: 'bert0.dataSetDescription'}));
    if (window.parent.dataset==-1) {
	document.getElementById("bert0.dataSetDescription").style.display="none";
    }
    else {
	
	document.getElementById("bert0.dataSetDescription").style.display='table-row';}
	document.getElementById('bert0.loadDataSet').value=window.parent.dataset;
    
}

function Dataset(my_action,n){
    var my_item=document.getElementById(my_action);
	if (my_item.value>=n && my_item.value<=19) {
	    if(n==1){ 
		Check = prompt("Enter descrition.", "");
		if (Check==null) {return; }
		if (Check!="") {
		    var my_item_d=document.getElementById('bert0.dataSetDescription');
		    my_item_d.value=Check;
		    window.parent.SaveVar(my_item_d,1,'',0);
		    document.getElementById('bert0.loadDataSet').value=my_item.value;
		    
		}
		else {alert("Description can not be empty!");
		return;};}
	    window.parent.SaveVar(my_item,1,'',0);
	    window.parent.window_onload();
	    window.parent.socket.send(JSON.stringify({get: 'bert0.dataSetDescription'}));
	    window.parent.dataset=my_item.value;
	    if (window.parent.dataset==-1) {
		document.getElementById("bert0.dataSetDescription").style.display="none";}
	    else {document.getElementById("bert0.dataSetDescription").style.display='table-row';}
	}
	else {alert("Wrong dataset number!"); }
 
}

function SaveAdmin(){
var pass = document.getElementById('system.passwdAdmin').value;

	
	var re = /[",',\\]/gi;                //      ',",\\
	var found = pass.match(re);
	if (found!=null){
	alert("Not: "+ found);	
	}
	else {
	
	window.parent.SaveVar(document.getElementById('system.passwdAdmin'),1,'',0);
	
	}
}

function RunScript(){
//  var script=document.getElementById("system.execScript").value;
  
  window.parent.SaveVar(document.getElementById('system.execScript'),1,'',0);  
    
}

function validateImput(evt,Feld) {
	//the pressed key
	var theEvent = evt || window.event;
	var bkey = theEvent.keyCode || theEvent.which;
	key = String.fromCharCode( bkey );
	console.log("theEvent:"+theEvent+" key: "+bkey);
	
	//allow arrow keys
	if ( bkey == 39 || bkey == 37 ) return;
	
	//allow backspace and color save indicator red
	if (bkey == 8){
		makeSaveVisible(Feld);
		return;
	}
	
	var s=Feld.value;
	var regex = /[a-zA-Z]/;               // /[-.0-9]/;
	if( regex.test(key) || s.length>=5 || bkey == 188) {
		theEvent.returnValue = false;
		if(theEvent.preventDefault) theEvent.preventDefault();
		return;
	}
	
	makeSaveVisible(Feld);
}
//**********************************************************    
function  makeSaveVisible(Feld){
	var but=(Feld.id+'_But');   
	//my_Inter=clearInterval(my_Inter);	
	document.getElementById(but).style.display='table-row';	
}

function TestpwrRef(item){

var myname=item.id;
myname=myname.substr(0,myname.length-4);
var elem=document.getElementById(myname);
var val=elem.value;
if (!isNumber(val) && val<=10 && val>=-10) {	
elem.value="";
}
else {
elem.value=Math.round(Number(val) * 100) / 100;    
window.parent.SaveVar(elem,1,'.L',0);  //
document.getElementById("mzMod0.Ki_But").style.display="none";
document.getElementById("mzMod1.Ki_But").style.display="none";
document.getElementById("mzMod2.Ki_But").style.display="none";
document.getElementById("mzMod3.Ki_But").style.display="none";
//StartInterval();
}
}
function isNumber(n) {
  return !isNaN(parseFloat(n)) && isFinite(n);
}