function verifyIP(ipaddr)
{
	var re = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;
   	if( re.test(ipaddr) )
   	{
    	var parts = ipaddr.split(".");
      	if( parseInt(parseFloat(parts[0])) == 0) 
      	{
      		return false;
      	}
      	if( parseInt(parseFloat(parts[3])) == 0 || parseInt(parseFloat(parts[3])) == 255 ) 
      	{
      		return false;
      	}
      	for(var i=0; i<parts.length; i++)
      	{
         	if( parseInt(parseFloat(parts[i])) > 255 )
         	{
         		return false;
         	}
      	}
      	return true;
   	}
   	else
   	{
    	return false;
   	}
}

function verifyNetmask(ipaddr)
{
	var re = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;
   	if( re.test(ipaddr) )
   	{
    	var parts = ipaddr.split(".");
      	if( parseInt(parseFloat(parts[0])) == 0) 
      	{
      		return false;
      	}
      	for(var i=0; i<parts.length; i++)
      	{
         	if( parseInt(parseFloat(parts[i])) > 255 )
         	{
         		return false;
         	}
      	}
      	return true;
   	}
   	else
   	{
    	return false;
   	}
}

function isValidPort(inpString)
{
 	if( /^\d{1,5}$/.test(inpString) == true )
 	{
 		if( inpString < 0 || inpString > 65535 )
 			return false;
		return true;
 	}
 	return false;
}

function isValidNumber(inpString)
{
  	return /^\d{1,5}$/.test(inpString);
}

function isValidEmail(inpString)
{
  	return /^.+\@(\[?)[a-zA-Z0-9\-\.]+\.([a-zA-Z]{2,3}|[0-9]{1,3})(\]?)$/.test(inpString);
}
