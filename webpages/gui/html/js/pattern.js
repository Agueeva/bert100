var my_item;
var i;
function Check_Auto(n){
   
 if (document.getElementById('all').checked) { 
 AUTO_main_0();
 AUTO_main();      
}
else {
 AUTO_main_eins(n);
} 

	if (n==0) {
	i=2;	
	}
	else {
		i=1;
		}
 var my_itemA=document.getElementById('cdr0.l'+n+'.prbs_autovr');

if (my_itemA.value==1) {
	my_item=document.getElementById('bert0.L'+n+'.patVerSel');
        my_item.value=document.getElementById('bert0.L'+n+'.prbsPatGenSel').value;
	my_item.disabled=true;
	window.parent.SaveVar(my_item,i,'.L',0);
        window.parent.window_onload();
}
else {
document.getElementById('bert0.L'+n+'.patVerSel').disabled=false;
}
}
//window.parent.myElement=window.parent.myVarPattern;
//  window.parent.SaveVar(this,2,'.l',0); window.parent.window_onload();


function all_load() {
   main_AUTO();
var my_all=document.getElementById('all');
//alert(window.parent.all_pat);
if (window.parent.all_pat) {
	 my_all.checked=true;
         window.parent.myElement=window.parent.myVarPattern0;
         window.parent.myDisableAll(my_all,'bert0.L','.L',0);
        // window.parent.myElement=window.parent.myVarPattern1;
         window.parent.myDisableAuto();
         window.parent.myElement=window.parent.myVarPattern;
         AUTO_main_all_ch();
}
else {
    AUTO_main_all_on();
for (i = 0; i <= 3; i++){
    var my_itemA=document.getElementById('cdr0.l'+i+'.prbs_autovr');
	my_item=document.getElementById('bert0.L'+i+'.patVerSel');
	my_item.disabled=true;
}}
}
function fun_all(item){
   // alert(item.checked);
window.parent.all_pat=item.checked;
//alert(window.parent.all_pat);
window.parent.myElement=window.parent.myVarPattern0;
window.parent.myCheckAll(item,'bert0.L','.L',0);
//window.parent.myElement=window.parent.myVarPattern1;
//window.parent.myCheckAll(item,'cdr0.l','.l',0);
window.parent.myElement=window.parent.myVarPattern;
window.parent.window_onload();
if (item.checked) {
 AUTO_main_0();
 AUTO_main_all_ch();
}
else {AUTO_main_all_on();}
AUTO_main();
}
function AUTO_main_0() {
  var my_wert=document.getElementById('cdr0.l0.prbs_autovr').value;
    document.getElementById('cdr0.l1.prbs_autovr').value=my_wert;
    document.getElementById('cdr0.l2.prbs_autovr').value=my_wert;
    document.getElementById('cdr0.l3.prbs_autovr').value=my_wert; }
function AUTO_main_all_ch() {
    document.getElementById('cdr0.l1.prbs_autovr').disabled=true;
    document.getElementById('cdr0.l2.prbs_autovr').disabled=true;
    document.getElementById('cdr0.l3.prbs_autovr').disabled=true; }
    
function AUTO_main_all_on() {
    document.getElementById('cdr0.l1.prbs_autovr').disabled=false;
    document.getElementById('cdr0.l2.prbs_autovr').disabled=false;
    document.getElementById('cdr0.l3.prbs_autovr').disabled=false; }
    
function AUTO_main() {
    window.parent.prbs_autovr0=document.getElementById('cdr0.l0.prbs_autovr').value;
    window.parent.prbs_autovr1=document.getElementById('cdr0.l1.prbs_autovr').value;
    window.parent.prbs_autovr2=document.getElementById('cdr0.l2.prbs_autovr').value;
    window.parent.prbs_autovr3=document.getElementById('cdr0.l3.prbs_autovr').value;}
    
    function main_AUTO() {
    document.getElementById('cdr0.l0.prbs_autovr').value=window.parent.prbs_autovr0;
    document.getElementById('cdr0.l1.prbs_autovr').value=window.parent.prbs_autovr1;
    document.getElementById('cdr0.l2.prbs_autovr').value=window.parent.prbs_autovr2;
    document.getElementById('cdr0.l3.prbs_autovr').value=window.parent.prbs_autovr3;}
    
   function  AUTO_main_eins(n){

   switch (n) {
        case 0 :
            window.parent.prbs_autovr0=document.getElementById('cdr0.l0.prbs_autovr').value;
        break;
        case 1 :
            window.parent.prbs_autovr1=document.getElementById('cdr0.l1.prbs_autovr').value;
            break;
        case 2 :
            window.parent.prbs_autovr2=document.getElementById('cdr0.l2.prbs_autovr').value;
            break;
        case 3 :
            window.parent.prbs_autovr3=document.getElementById('cdr0.l3.prbs_autovr').value;
        break;
        default:
        break;
    } 
    
    }
    
    