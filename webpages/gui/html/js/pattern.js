var my_item;
	var i;
function Check_Auto(n){
	
	if (n==0) {
	i=2;	
	}
	else {
		i=1;
		}
 my_item=document.getElementById('cdr0.l'+n+'.prbs_autovr');
if (my_item.value==1) {
	my_item=document.getElementById('bert0.L'+n+'.patVerSel');
        my_item.value=document.getElementById('bert0.L'+n+'.patVerSel').value;
	my_item.disabled=true;
	window.parent.SaveVar(my_item,i,'.L',0);
}
else {
document.getElementById('bert0.L'+n+'.patVerSel').disabled=false;	
}

	
}  
var my_all=document.getElementById('all');

if (window.parent.all_pat) {
	my_all.checked=true;
	 window.parent.myDisableAll(my_all,'bert0.L','.L',0)
}
for (i = 0; i <= 3; i++){
my_item=document.getElementById('bert0.L'+i+'.prbs_autovr');
if (my_item.value==1) {
	my_item=document.getElementById('bert0.L'+i+'.patVerSel');
	my_item.disabled=true;
}}

function fun_all(item){
window.parent.all_pat=item.checked;
window.parent.myElement=window.parent.myVarPattern0;
window.parent.myCheckAll(item,'bert0.L','.L',0);
window.parent.myElement=window.parent.myVarPattern1;
window.parent.myCheckAll(item,'cdr0.l','.l',0);
window.parent.myElement=window.parent.myVarPattern;
window.parent.window_onload();
    
}