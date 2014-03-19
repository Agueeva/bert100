
function ClickUp(i,ud,sr,k){
    var my_item=document.getElementById('emlAmp'+i+'.vg1');
    switch(ud){
        case 1:
            if((Number(my_item.value)+sr*0.01) < 3) {
               my_item.value=Math.round((Number(my_item.value)+(sr*0.01))* 100) / 100;
            }else {
               my_item.value=3;
            }
        break;
        case 2:
            if((my_item.value-sr*0.01) > 1.8) {
                my_item.value=Math.round((Number(my_item.value)-(sr*0.01))* 100) / 100;
            }else{
                my_item.value=1.8;
            }
        break;
        default:
        break;
    }
TestVal(i);
my_item=document.getElementById('emlAmp'+i+'.vg1');
window.parent.SaveVar(my_item,k,'Amp',1);
if (k==2) {
  window.parent.myElement=window.parent.myVarTX;
  window.parent.window_onload();
}
}
//emlAmp1.vg1
function TestVal(i){
	var my_item=document.getElementById('emlAmp'+i+'.vg1');
	if (isNaN(my_item.value)) {
		
	}else {
		
		if(my_item.value < 1.8) { my_item.value=1.8;}
		if(my_item.value > 3) { my_item.value=3;}
                
	}
}


// Crossing
function ClickUpCr(i,ud,sr,k){
var my_item=document.getElementById('emlAmp'+i+'.vg2');
switch(ud){
    case 1:
        if((Number(my_item.value)+sr*0.01) < 1.5) {
            my_item.value=Math.round((Number(my_item.value)+(sr*0.01)) * 100) / 100;
        }else {
            my_item.value=1.5; }
    break;
    case 2:
        if((my_item.value-sr*0.01) > 0) {
            my_item.value=Math.round((Number(my_item.value)-(sr*0.01)) * 100) / 100;
        }else{
            my_item.value=0;
        }
    break;
    default:
    break;
}
window.parent.SaveVar(my_item,k,'Amp',1);
if (k==2) {
  window.parent.myElement=window.parent.myVarTX;
  window.parent.window_onload();
}
}

function TestValCr(i){
var my_item=document.getElementById('emlAmp'+i+'.vg2');
 	var item = "emlAmp"+i+".vg1";
	if (isNaN(my_item.value)) {
		
	}else {
		
		if(my_item.value < 0) { my_item.value=0;}
		if(my_item.value > 1.5) { my_item.value=1.5;}
}

}
function Check_all() {
    
var my_all=document.getElementById('all');

if (window.parent.all_tx_opt_test) {
	my_all.checked=true;
	window.parent.myElement=window.parent.myVarTX_eml_test0;
	window.parent.myDisableAll(my_all,'emlAmp','Amp',0);
	window.parent.myElement=window.parent.myVarTX_eml_test2;
	window.parent.myDisableAll(my_all,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarTX_eml_test;
}
/*window.parent.socket.send(JSON.stringify({get: 'bert0.dataSetDescription'}));
    if (window.parent.dataset==-1) {
	document.getElementById("bert0.dataSetDescription").style.display="none";
    }
    else {
	
	document.getElementById("bert0.dataSetDescription").style.display='table-row';}
	document.getElementById('bert0.loadDataSet').value=window.parent.dataset;
 */   
}



function Dataset(my_action,n){
    var my_item=document.getElementById(my_action);
	if (my_item.value>=n && my_item.value<=19) {
	    if(n==1){
		var my_item_d=document.getElementById('bert0.dataSetOutAmpl');
		 if(!isNumber( my_item_d.value)){my_item_d.value=''; return;}
		    window.parent.SaveVar(my_item_d,1,'',0);
		Check = prompt("Enter descrition.", "");
		if (Check==null) {return; }
		if (Check!="") {
		    my_item_d=document.getElementById('bert0.dataSetDescription');
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
	/*    if (window.parent.dataset==-1) {
		document.getElementById("bert0.dataSetDescription").style.display="none";}
	    else {document.getElementById("bert0.dataSetDescription").style.display='table-row';}*/
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


function isNumber(n) {
  return !isNaN(parseFloat(n)) && isFinite(n);
}