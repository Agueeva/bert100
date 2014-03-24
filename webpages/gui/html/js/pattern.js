var my_item;
var i;
//**********************************************************
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
 var my_itemA=document.getElementById('l'+n+'.prbs_autovr');

if (my_itemA.value==1) {
	my_item=document.getElementById('bert0.L'+n+'.patVerSel');
        my_item.value=document.getElementById('bert0.L'+n+'.prbsPatGenSel').value;
	my_item.disabled=true;
        if (document.getElementById('bert0.L'+n+'.prbsPatGenSel').value!=3 && document.getElementById('bert0.L'+n+'.prbsPatGenSel').value!=2) {
	window.parent.SaveVar(my_item,i,'.L',0); }
        window.parent.window_onload();
}
else {
document.getElementById('bert0.L'+n+'.patVerSel').disabled=false;
}
}
//**********************************************************
function all_load() {
   main_AUTO();
var my_all=document.getElementById('all');
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
  //  var my_itemA=document.getElementById('l'+i+'.prbs_autovr');
	my_item=document.getElementById('bert0.L'+i+'.patVerSel');
	my_item.disabled=true;
}}
for (i = 0; i <= 3; i++){
 my_item=document.getElementById('bert0.L'+i+'.prbsPatGenSel');
 if ( my_item.value==3) {
   document.getElementById('bert0.L'+i+'.patVerSel').disabled=true;
   document.getElementById('l'+i+'.prbs_autovr').disabled=true;
 }
}
}

//**********************************************************
function fun_all(item){
	window.parent.all_pat=item.checked;
	window.parent.myElement=window.parent.myVarPattern0;
	window.parent.myCheckAll(item,'bert0.L','.L',0);
	window.parent.myElement=window.parent.myVarPattern;
	window.parent.window_onload();
	if (item.checked) {
		AUTO_main_0();
		AUTO_main_all_ch();
}
	else {AUTO_main_all_on();}
		AUTO_main();
       showFun(false);         
                
}

//**********************************************************
function AUTO_main_0() {
  var my_wert=document.getElementById('l0.prbs_autovr').value;
    document.getElementById('l1.prbs_autovr').value=my_wert;
    document.getElementById('l2.prbs_autovr').value=my_wert;
    document.getElementById('l3.prbs_autovr').value=my_wert; }
    
//**********************************************************    
function AUTO_main_all_ch() {
    document.getElementById('l1.prbs_autovr').disabled=true;
    document.getElementById('l2.prbs_autovr').disabled=true;
    document.getElementById('l3.prbs_autovr').disabled=true; }

//**********************************************************    
function AUTO_main_all_on() {
    document.getElementById('l1.prbs_autovr').disabled=false;
    document.getElementById('l2.prbs_autovr').disabled=false;
    document.getElementById('l3.prbs_autovr').disabled=false; }
//**********************************************************    
function AUTO_main() {
    window.parent.prbs_autovr0=document.getElementById('l0.prbs_autovr').value;
    window.parent.prbs_autovr1=document.getElementById('l1.prbs_autovr').value;
    window.parent.prbs_autovr2=document.getElementById('l2.prbs_autovr').value;
    window.parent.prbs_autovr3=document.getElementById('l3.prbs_autovr').value;}
    
    function main_AUTO() {
    document.getElementById('l0.prbs_autovr').value=window.parent.prbs_autovr0;
    document.getElementById('l1.prbs_autovr').value=window.parent.prbs_autovr1;
    document.getElementById('l2.prbs_autovr').value=window.parent.prbs_autovr2;
    document.getElementById('l3.prbs_autovr').value=window.parent.prbs_autovr3;}
//**********************************************************    
   function  AUTO_main_eins(n){

   switch (n) {
        case 0 :
            window.parent.prbs_autovr0=document.getElementById('l0.prbs_autovr').value;
        break;
        case 1 :
            window.parent.prbs_autovr1=document.getElementById('l1.prbs_autovr').value;
            break;
        case 2 :
            window.parent.prbs_autovr2=document.getElementById('l2.prbs_autovr').value;
            break;
        case 3 :
            window.parent.prbs_autovr3=document.getElementById('l3.prbs_autovr').value;
        break;
        default:
        break;
    } 
    
    }
    
//****************************************    
   
//**********************************************************
function Check_Pattern(L,element,n)
{
	if (element.value=="3") {
		window.parent.ReadVarByName("bert0.userPattern");
		window.parent.SaveVar(element,n,'.L',0);
		//document.getElementById('l'+L+'.prbs_autovr').value=0;
                document.getElementById('l'+L+'.prbs_autovr').disabled=true;
                document.getElementById('bert0.L'+L+'.patVerSel').disabled=true;
                if (window.parent.all_pat) {
                    AUTO_main_all_ch()
                }
		showFun( true);
	}
	else if (element.value=="2") {
	showFun(false);
	window.parent.SaveVar(element,n,'.L',0);
	document.getElementById('l'+L+'.prbs_autovr').value=0;
        document.getElementById('l'+L+'.prbs_autovr').disabled=false;
	Check_Auto(L);
	}
	else {
		showFun(false);
		window.parent.SaveVar(element,n,'.L',0);
		document.getElementById('bert0.L'+L+'.prbsPatGenSel').value=element.value;
		window.parent.SaveVar(document.getElementById('bert0.L'+L+'.prbsPatGenSel'),n,'.L',0);
                document.getElementById('l'+L+'.prbs_autovr').disabled=false;
                Check_Auto(L);
	}
}


//**********************************************************
function showFun(visible) {
    if(visible) {
	document.getElementById('userPattern0').style.display = 'table-row';
	
	

    } else {
if ((document.getElementById('bert0.L0.prbsPatGenSel').value!=3 && document.getElementById('bert0.L1.prbsPatGenSel').value!=3 && document.getElementById('bert0.L2.prbsPatGenSel').value!=3 && document.getElementById('bert0.L3.prbsPatGenSel').value!=3)
    || (document.getElementById('all').checked && document.getElementById('bert0.L0.prbsPatGenSel').value!=3)) {
	

	document.getElementById('userPattern0').style.display = 'none';
	
    }}
}
//**********************************************************
function binStringToHex(s) {
       var s2 = '', c;
       s2= "0x"+ parseInt( s, 2).toString(16);
        
        return s2;
}
//**********************************************************
function TestUserPat() {
	
	var string = document.getElementById('userHex1').value + document.getElementById('userHex2').value + document.getElementById('userHex3').value+document.getElementById('userHex4').value;
	
	var re = /[2-9]/gi;
	var found = string.match(re);
	var savePatternIndicatorElement = document.getElementById('savePatternIndicator');
	if (string.length!=40 ||found!=null){
		alert("Length="+string.length +" OR found: "+ found);
		savePatternIndicatorElement.innerHTML = "NOT Saved!";
		savePatternIndicatorElement.className = "saveindicatorRed";
	}else {
		string=binStringToHex(string);
		document.getElementById('bert0.userPattern').value=string;
		window.parent.SaveVar(document.getElementById('bert0.userPattern'),1,'',0);
		//window.parent.all_pat=true;
	        //document.getElementById('all').checked=true;
		//Check_Auto(0);
		savePatternIndicatorElement.innerHTML = "Saved!";
		savePatternIndicatorElement.className = "saveindicatorGreen";
		//savePatternIndicatorElement.style.display = 'block';
	}
}

/*
//Old validateImput
function PruefeInhalt(Feld) {
	var s=Feld.value;
	var s2=s.substr(s.length-1,1);
	
	//change the save indicator to red
	makeSavePatternIndicatorRed();
	
	//remove everything but for "0" and "1"
	s=s.replace(/[^0-1]+/,'');
	
	//check length of string
	if( s.length>40){
		Feld.value=s.substring(0,40);	
	}else{
		Feld.value=s;	
	}
}
*/
//**********************************************************
function validateImput(evt,Feld) {
	//the pressed key
	var theEvent = evt || window.event;
	var bkey = theEvent.keyCode || theEvent.which;
	key = String.fromCharCode( bkey );
	//console.log("theEvent:"+theEvent+" key: "+bkey);
	
	//allow arrow keys
	if ( bkey == 39 || bkey == 37) return;
	
	//allow backspace and color save indicator red
	if (bkey == 8){
		makeSavePatternIndicatorRed();
		return;
	}
	
	//allow only "0" and "1" and check length of string
	var s=Feld.value;
	var regex = /[0-1]/;
	if( !regex.test(key) || s.length>=10) {
		theEvent.returnValue = false;
		if(theEvent.preventDefault) theEvent.preventDefault();
		return;
	}
	
	makeSavePatternIndicatorRed();
}
//**********************************************************    
function  makeSavePatternIndicatorRed(){
	//change the save indicator to red
	var savePatternIndicatorElement = document.getElementById('savePatternIndicator');
	savePatternIndicatorElement.innerHTML = "NOT Saved!";
	savePatternIndicatorElement.className = "saveindicatorRed";
}