document.getElementById("tx0.pwrRef_But").style.display="none";
document.getElementById("tx1.pwrRef_But").style.display="none";
document.getElementById("tx2.pwrRef_But").style.display="none";
document.getElementById("tx3.pwrRef_But").style.display="none";

document.getElementById("mzMod0.bias_But").style.display="none";
document.getElementById("mzMod1.bias_But").style.display="none";
document.getElementById("mzMod2.bias_But").style.display="none";
document.getElementById("mzMod3.bias_But").style.display="none";	
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
	my_Inter=clearInterval(my_Inter);	
	document.getElementById(but).style.display='table-row';	
}

function TestpwrRef(item){

var myname=item.id;
myname=myname.substr(0,myname.length-4);
var elem=document.getElementById(myname);
var val=elem.value;
if (!isNumber(val)) {	
elem.value="";
}
else {
window.parent.SaveVar(elem,1,'.L',0);  //tx0.pwrRef_But   mzMod0.bias_But
document.getElementById("tx0.pwrRef_But").style.display="none";
document.getElementById("tx1.pwrRef_But").style.display="none";
document.getElementById("tx2.pwrRef_But").style.display="none";
document.getElementById("tx3.pwrRef_But").style.display="none";
document.getElementById("mzMod0.bias_But").style.display="none";
document.getElementById("mzMod1.bias_But").style.display="none";
document.getElementById("mzMod2.bias_But").style.display="none";
document.getElementById("mzMod3.bias_But").style.display="none";
StartInterval();
}
}
function isNumber(n) {
  return !isNaN(parseFloat(n)) && isFinite(n);
}