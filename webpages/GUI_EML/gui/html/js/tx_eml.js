
function ClickUp(suv,prev,i,ud,sr,k,max,min){
    var my_item=document.getElementById(suv+i+prev);
    console.log("item:"+suv+i+prev);
    switch(ud){
        case 1:
            if((Number(my_item.value)+sr*0.01) < max) {
               my_item.value=Math.round((Number(my_item.value)+(sr*0.01))* 1000) / 1000;
            }else {
               my_item.value=max;
            }
        break;
        case 2:
            if((my_item.value-sr*0.01) > min) {
                my_item.value=Math.round((Number(my_item.value)-(sr*0.01))* 100) / 100;
            }else{
                my_item.value=min;
            }
        break;
        default:
        break;
    }
TestVal(suv,prev,i,max,min);
window.parent.SaveVar(my_item,k,'Amp',0);
switch(i){
        case 0:
	    window.parent.outAmpl0=document.getElementById('bert0.outAmpl0').value;
	break;
       case 1:
	    window.parent.outAmpl1=document.getElementById('bert0.outAmpl1').value;
	break;
	case 2:
	    window.parent.outAmpl2=document.getElementById('bert0.outAmpl2').value;
	break;
	case 3:
	    window.parent.outAmpl3=document.getElementById('bert0.outAmpl3').value;
	break;
}
window.parent.myElement=window.parent.myVarTX;
window.parent.window_onload();
}
//emlAmp1.vg1
function TestVal(suv,prev,i,max,min){
	var my_item=document.getElementById(suv+i+prev);
	if (isNaN(my_item.value)) {
		
	}else {
		
		if(my_item.value < min) { my_item.value=min;}
		if(my_item.value > max) { my_item.value=max;}
                
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

if (window.parent.all_tx_opt) {
	my_all.checked=true;
	window.parent.myElement=window.parent.myVarTX0;
	window.parent.myDisableAll(my_all,'emlAmp','Amp',0);
	window.parent.myElement=window.parent.myVarTX2;
	window.parent.myDisableAll(my_all,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarTX;
}
document.getElementById("bert0.outAmpl0").value=window.parent.outAmpl0;
document.getElementById("bert0.outAmpl1").value=window.parent.outAmpl1;
document.getElementById("bert0.outAmpl2").value=window.parent.outAmpl2;
document.getElementById("bert0.outAmpl3").value=window.parent.outAmpl3;

}
function CheckDataSet(){
  window.parent.socket.send(JSON.stringify({get: 'bert0.dataSetDescription'}));
   // if (window.parent.dataset==-1) {
	document.getElementById("bert0.dataSetDescription").style.display="none";
  //  }
  //  else {
	document.getElementById("bert0.dataSetDescription").style.display='table-row';  //}
	//document.getElementById('bert0.loadDataSet').value=window.parent.dataset;    
}

function Dataset(my_action,n){
    var my_item=document.getElementById(my_action);
	if (my_item.value>=n && my_item.value<=19) {
	    window.parent.SaveVar(my_item,1,'',0);
	    window.parent.window_onload();
	    window.parent.socket.send(JSON.stringify({get: 'bert0.dataSetDescription'}));
	    window.parent.dataset=my_item.value;
	  //  if (window.parent.dataset==-1) {
		//document.getElementById("bert0.dataSetDescription").style.display="none";}
	  //  else {
		document.getElementById("bert0.dataSetDescription").style.display='table-row';}
	//}
	else {alert("Wrong dataset number!"); }
 
}


