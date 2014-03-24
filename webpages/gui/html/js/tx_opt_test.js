
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
window.parent.SaveVar(my_item,k,'amp',1);
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
	window.parent.myDisableAll(my_all,'amp','amp',1);
	window.parent.myElement=window.parent.myVarTX_opt_test2;
	window.parent.myDisableAll(my_all,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarTX_opt_test;
}
}