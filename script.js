// Because we want to access DOM node,
// we initialize our script at page load.
window.addEventListener('load', function () {

  // This variables will be used to store the form data
  var file = {
        dom    : document.getElementById("i2"),
        binary : null,
        length : 0
      };
 var data     = "";
  // We use the FileReader API to access our file content
  var reader = new FileReader();

  // Because de FileReader API is asynchronous, we need
  // to store it's result when it has finish to read the file
  reader.addEventListener("load", function () {
    file.binary = reader.result;
  });

  // At page load, if a file is already selected, we read it.
  if(file.dom.files[0]) {
    reader.readAsBinaryString(file.dom.files[0]);
	file.length = reader.length;
  }

  // However, we will read the file once the user selected it.
  file.dom.addEventListener("change", function () {
    if(reader.readyState === FileReader.LOADING) {
      reader.abort();
    }
    
    reader.readAsBinaryString(file.dom.files[0]);
	file.length = reader.length;
  });

  // The sendData function is our main function
  function sendData() {
    // At first, if there is a file selected, we have to wait it is read
    // If it is not, we delay the execution of the function
    if(!file.binary && file.dom.files.length > 0) {
      setTimeout(sendData, 10);
      return;
    }

	// To construct our multipart form data request,
    // We need an XMLHttpRequest instance
    var XHR = new XMLHttpRequest();
	
	// We need a sperator to define each part of the request
    var boundary = "blob";
	var filePosition = 0;
	var partNumber = 1;
	var maxPacketSize = 60000;
	
	XHR.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			// If there is still data to be sent we continue.
			if(this.responseText == 'continue' && filePosition < file.binary.length)
			{
				CreateBodyRequest();
			}
			else
			{
				alert(this.responseText);
			}
		}
		else if (this.readyState == 4)
		{
			alert("No answer when sending data. Please try again.");
		}
    };
	
	// So, if the user has selected a file
    if (file.dom.files[0]) {
		//console.log("File selected");
		//console.log(file.binary.length);
		
		if(filePosition < file.binary.length)
		{
			CreateBodyRequest();
		}
	}	
	
	function CreateBodyRequest()
	{
		data = "";
			
		// We start a new part in our body's request
		data += "------" + boundary + "\r\n";

		// We said it's form data (it could be something else)
		data += 'content-disposition: form-data; '
		// We define the name of the form data
		+ 'name="'         + file.dom.name          + '"; '
		// We provide the real name of the file
		+ 'filename="'     + file.dom.files[0].name + '"\r\n';
		// We provide the MIME type of the file
		data += 'Content-Type: ' + file.dom.files[0].type + '\r\n';
		data += 'FileSize: ' + file.binary.length + '\r\n';
		data += 'Part: ' + partNumber + '\r\n';
		partNumber += 1;
		// There is always a blank line between the meta-data and the data
		data += '\r\n';
	
		// We happen the binary data to our body's request
		var toRead = 0;
		if(file.binary.length - filePosition > maxPacketSize)
		{
			toRead = maxPacketSize;
		}
		else
		{
			toRead = file.binary.length - filePosition;
		}
		data += file.binary.substr(filePosition, toRead);  
		filePosition += toRead;
	
		data += '\r\n' + '------' + boundary + '--' + '\r\n';
		
		// We setup our request
		XHR.open("POST", "http://192.168.0.1/upgrade");
		// We add the required HTTP header to handle a multipart form data POST request
		XHR.setRequestHeader('Content-Type','multipart/form-data; boundary=' + boundary);
		XHR.sendAsBinary(data);
	};
  }

  // At least, We need to access our form
  //var form   = document.getElementById("myForm");

  // to take over the submit event
  //form.addEventListener('submit', function (event) {
  //  event.preventDefault();
  //  sendData();
  //);
  document.getElementById("myForm").addEventListener("submit", function(event){
    event.preventDefault();
	sendData();
	});
	
	
	
	XMLHttpRequest.prototype.sendAsBinary = function(datastr) {
    function byteValue(x) {
        return x.charCodeAt(0) & 0xff;
    }
    var ords = Array.prototype.map.call(datastr, byteValue);
    var ui8a = new Uint8Array(ords);
    this.send(ui8a.buffer);
};
});