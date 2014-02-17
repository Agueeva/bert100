
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

