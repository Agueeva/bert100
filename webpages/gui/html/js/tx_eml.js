
function ClickUp(i,ud,sr,k){
    var my_item=document.getElementById('emlAmp'+i+'.vg1');
    switch(ud){
        case 1:
            if((Number(my_item.value)+sr*0.01) < 2.8) {
               my_item.value=Math.round((Number(my_item.value)+(sr*0.01))* 100) / 100;
            }else {
               my_item.value=2.8;
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
		if(my_item.value > 2.8) { my_item.value=2.8;}
                
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

if (window.parent.all_tx) {
	my_all.checked=true;
	window.parent.myElement=window.parent.myVarTX0;
	window.parent.myDisableAll(my_all,'emlAmp','Amp',1);
	window.parent.myElement=window.parent.myVarTX2;
	window.parent.myDisableAll(my_all,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarTX;
}
}