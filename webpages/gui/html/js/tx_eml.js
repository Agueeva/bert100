
function ClickUp(i,ud,sr,k){
    var my_item=document.getElementById('emlAmp'+i+'.vg1');
    switch(ud){
        case 1:
            if((Number(my_item.value)+sr*0.01) < 2.6) {
               my_item.value=Math.round((Number(my_item.value)+(sr*0.01))* 100) / 100;
            }else {
               my_item.value=2.6;
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
}



// Crossing
function ClickUpCr(i,ud,sr,k){
var my_item=document.getElementById('emlAmp'+i+'.vg2');
switch(ud){
    case 1:
        if((Number(my_item.value)+sr*0.01) < 1) {
            my_item.value=Math.round((Number(my_item.value)+(sr*0.01)) * 100) / 100;
        }else {
            my_item.value=1; }
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
}

function TestValCr(i){
// value=Math.round(value * 100) / 100;
var my_item=document.getElementById('emlAmp'+i+'.vg2');
 	var item = "emlAmp"+i+".vg1";
	//alert($("#"+item.replace(/[.]/g,'\\.')));
	if (isNaN(my_item.value)) {
		//$("#"+item.replace(/[.]/g,'\\.')).addClass("textfild_error");
		
	}else {
		$("#"+item.replace(/[.]/g,'\\.')).removeClass("textfild_error");
		if(my_item.value < 0) { my_item.value=0;}
		if(my_item.value > 1) { my_item.value=1;}
}

}

var my_all=document.getElementById('all');

if (window.parent.all_tx) {
	my_all.checked=true;
	window.parent.myElement=window.parent.myVarTX0;
	window.parent.myDisableAll(my_all,'emlAmp','Amp',1);
	window.parent.myElement=window.parent.myVarTX1;
	window.parent.myDisableAll(my_all,'cdr0.l','.l',0);
	window.parent.myElement=window.parent.myVarTX;
}