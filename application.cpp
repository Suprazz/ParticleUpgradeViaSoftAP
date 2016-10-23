#pragma SPARK_NO_PREPROCESSOR

#include "Particle.h"
#include "softap_http.h"
#include "system_update.h"
#include "myFileSystem.h"

uint16_t destinationFile;

#define UPGRADE_FILE_NAME "upgrade"
SYSTEM_THREAD(ENABLED);

struct Page
{
    const char* url;
    const char* mime_type;
    const char* data;
};

const char index_html[] = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Firmware upgrade</title><link rel='stylesheet' type='text/css' href='style.css'></head><body> <p><b>Select file for firmware upgrade</b></p> <form id=\"myForm\"> <p> <input id=\"i2\" name=\"myFile\" type=\"file\"> </p> <input type=\"submit\" value=\"Begin upgrade\" id=\"submit\"/></form><script src='script.js'></script></body></html>";
const char style_css[] = "html{height:100%;margin:auto;background-color:white}body{box-sizing:border-box;min-height:100%;padding:20px;background-color:#1aabe0;font-family:'Lucida Sans Unicode','Lucida Grande',sans-serif;font-weight:normal;color:white;margin-top:0;margin-left:auto;margin-right:auto;margin-bottom:0;max-width:400px;text-align:center;border:1px solid #6e6e70;border-radius:4px}div{margin-top:25px;margin-bottom:25px}h1{margin-top:25px;margin-bottom:25px}button{border-color:#1c75be;background-color:#1c75be;color:white;border-radius:5px;height:30px;font-size:15px;font-weight:bold}button.input-helper{background-color:#bebebe;border-color:#bebebe;color:#6e6e70;margin-left:3px}button:disabled{background-color:#bebebe;border-color:#bebebe;color:white}input[type='text'],input[type='password']{background-color:white;color:#6e6e70;border-color:white;border-radius:5px;height:25px;text-align:center;font-size:15px}input:disabled{background-color:#bebebe;border-color:#bebebe}input[type='radio']{position:relative;bottom:-0.33em;margin:0;border:0;height:1.5em;width:15%}label{padding-top:7px;padding-bottom:7px;padding-left:5%;display:inline-block;width:80%;text-align:left}input[type='radio']:checked+label{font-weight:bold;color:#1c75be}.scanning-error{font-weight:bold;text-align:center}.radio-div{box-sizing:border-box;margin:2px;margin-left:auto;margin-right:auto;background-color:white;color:#6e6e70;border:1px solid #6e6e70;border-radius:3px;width:100%;padding:5px}#networks-div{margin-left:auto;margin-right:auto;text-align:left}#device-id{text-align:center}#scan-button{min-width:100px}#connect-button{display:block;min-width:100px;margin-top:10px;margin-left:auto;margin-right:auto;margin-bottom:20px}#password{margin-top:20px;margin-bottom:10px}";
const char script_js[] = "window.addEventListener(\"load\",function(){function d(){function i(){b=\"\",b+=\"------\"+e+\"\\r\\n\",b+='content-disposition: form-data; name=\"'+a.dom.name+'\"; filename=\"'+a.dom.files[0].name+'\"\\r\\n',b+=\"Content-Type: \"+a.dom.files[0].type+\"\\r\\n\",b+=\"FileSize: \"+a.binary.length+\"\\r\\n\",b+=\"Part: \"+g+\"\\r\\n\",g+=1,b+=\"\\r\\n\";var d=0;d=a.binary.length-f>h?h:a.binary.length-f,b+=a.binary.substr(f,d),f+=d,b+=\"\\r\\n------\"+e+\"--\\r\\n\",c.open(\"POST\",\"http://192.168.0.1/upgrade\"),c.setRequestHeader(\"Content-Type\",\"multipart/form-data; boundary=\"+e),c.sendAsBinary(b)}if(!a.binary&&a.dom.files.length>0)return void setTimeout(d,10);var c=new XMLHttpRequest,e=\"blob\",f=0,g=1,h=6e4;c.addEventListener(\"load\",function(b){f<a.binary.length&&i()}),c.addEventListener(\"error\",function(a){alert(\"Oups! Something goes wrong.\")}),a.dom.files[0]&&f<a.binary.length&&i()}var a={dom:document.getElementById(\"i2\"),binary:null,length:0},b=\"\",c=new FileReader;c.addEventListener(\"load\",function(){a.binary=c.result}),a.dom.files[0]&&(c.readAsBinaryString(a.dom.files[0]),a.length=c.length),a.dom.addEventListener(\"change\",function(){c.readyState===FileReader.LOADING&&c.abort(),c.readAsBinaryString(a.dom.files[0]),a.length=c.length}),document.getElementById(\"myForm\").addEventListener(\"submit\",function(a){a.preventDefault(),d()}),XMLHttpRequest.prototype.sendAsBinary=function(a){function b(a){return 255&a.charCodeAt(0)}var c=Array.prototype.map.call(a,b),d=new Uint8Array(c);this.send(d.buffer)}});";
const char txtPlain[] = "text/plain";

Page myPages[] = {
     { "/index.html", "text/html", index_html },
     { "/style.css", "text/css", style_css },
	 { "/script.js", "application/javascript", script_js },
     { nullptr }
};

FileTransfer::Descriptor file;
char upgradeDevice = 0;
static int fileLength;
void ExitMyPage(ResponseCallback* cb, void* cbArg, Writer* result, const char * str)
{
	Serial.println(str);
	cb(cbArg, 0, 200, txtPlain, nullptr);
	result->write(str);
}

void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved)
{
	static int partCount;
	static int fileBytesReceived;

	String urlString = String(url);
    Serial.printlnf("handling page %s", url);
	
    if (strcmp(url,"/index")==0) {
        Serial.println("sending redirect");
        Header h("Location: /index.html\r\n");
        cb(cbArg, 0, 301, txtPlain, &h);
        return;
    }

	if (strcmp(url, myPages[0].url) == 0) {
		// Index
		partCount = 0;
	}

	if (strcmp(url, "/upgrade") == 0)
	{
		if (partCount == 0)
		{
			fileLength = 0;
			fileBytesReceived = 0;
		}

		// Here is the output I should get
		/*
		POST Data: ------blob
		Content-Disposition: form-data; name="myfile"; filename="binfile"
		Content-Type: application/octet-stream

		content file here... after a blank line!
		*/

		char * str;
		int dataPos;
		int webkitformboundaryLength;
		uint8_t contentData[1152]; // MUST BE 1152 long. DONT CHANGE THIS!

		int datalength = body->read(contentData, sizeof(contentData));
		Serial.printlnf("Length = %d, Bytes left: %d", datalength, body->bytes_left);
	
		// Check if the first caracters are what we are expecting.
		str = strstr((const char *)&contentData[0], "------"); 
		if (str == NULL)
		{

			ExitMyPage(cb, cbArg, result, "Unexpected data");
			Serial.write(contentData, datalength);
			return;
		}

		// Debugging
		//Serial.println("Debug:");
		//Serial.write(contentData, datalength);

		// Find length of first line : webkitformboundary....
		str = strstr((const char *)contentData, "\r\n");
		
		if (str != NULL)
		{
			// I need figure out the length of the last line so this is why I do this.
			webkitformboundaryLength = str - (char*)contentData; // Length of the first line.
			webkitformboundaryLength += 2; // Including the \r\n
			// For the ending line
			webkitformboundaryLength += 2; // 2 characters for the "--" padding on the final line.
			webkitformboundaryLength += 2; // For the \r\n before the ending line
			Serial.printlnf("Webkitform length = %d", webkitformboundaryLength);
		}
		else
		{
			ExitMyPage(cb, cbArg, result, "Cannot find the length of the first line...");
			return;
		}
	
		const char filesize[] = "FileSize: ";
		str = strstr((const char *)contentData, filesize);
		if (str != NULL)
		{
			
			str += sizeof(filesize)-1;
			sscanf(str, "%d", &fileLength);
			Serial.printlnf("Total size of file: %d", fileLength);
			if (fileLength > 60000) // Must match the size in the JS file.
			{
				//Serial.printlnf("Upgrade should be called again after that");
			}
		}

		const char part[] = "Part: ";
		str = strstr((const char *)contentData, part);
		if (str != NULL)
		{
			int prevPart = partCount;
			str += sizeof(part) - 1;
			sscanf(str, "%d", &partCount);
			Serial.printlnf("Part: %d", partCount);
			if (partCount != prevPart + 1)
			{
				// Problème
				Serial.printlnf("Issue with part: %d, prevpart: %d", partCount, prevPart);
			}
		}

// uncomment and complete to get the file name.
// 		str = strstr((const char *)contentData, "filename=");
// 		if(str != NULL)
// 		{
// 			dataPos = (str - (char*)contentData);
// 			Serial.printlnf("Index of filename: %d", dataPos);
// 			str = strstr((const char *)contentData, "\r\n");
// 			if (str != NULL)
// 			{
// 				
// 			}
// 			// read until \r\n and we have a complete filename.
// 		}
		
		str = strstr((const char *)contentData, "\r\n\r\n");
		if(str != NULL)
		{
			int32_t written;
			uint8_t tempBuffer[100];
			int tempBufferLength = 0;
			dataPos = (str - (char*)contentData);
			dataPos += 4;			
			//Serial.printlnf("Data start here: %d", dataPos);
			datalength -= dataPos;

			// I can validate the data if I want here before going further:
// 			typedef struct module_info_t {
// 				const void* module_start_address;   // the first byte of this module in flash  // 0x080a0000;
// 				const void* module_end_address;     // the last byte (exclusive) of this smodule in flash. 4 byte crc starts here. 
// 				uint8_t reserved;					// 0
// 				uint8_t reserved2;					// 0
// 				uint16_t module_version;            // 16 bit version  // 0x0004
// 				uint16_t platform_id;               // The platform this module was compiled for. // 6 for the photon
// 				uint8_t  module_function;           // The module function // 0x05 for the MODULE_FUNCTION_USER_PART
// 				uint8_t  module_index;				// default 1
// 				module_dependency_t dependency;
// 				uint32_t reserved3;
// 			} module_info_t;

			if (partCount > 1)
			{
				// Add data to existing file.
				destinationFile = myFileOpen(UPGRADE_FILE_NAME);
				// Seek to the end of the file!!!
				myFileSeek(destinationFile, 60000);
			}
			else
			{
				// Create new file
				destinationFile = myFileOpenEmpty(UPGRADE_FILE_NAME);
			}
			

			if (destinationFile < 0)
			{
				ExitMyPage(cb, cbArg, result, "Open error");
				return;
			}

			while (body->bytes_left)
			{
				written = myFileWrite(destinationFile, &contentData[dataPos], datalength);
				fileBytesReceived += written;
				if (written != datalength)
				{
					Serial.printlnf("Written: %d, datalength: %d", written, datalength);
					ExitMyPage(cb, cbArg, result, "Write error 1");
					return;

				}
				
				datalength = body->read(contentData, sizeof(contentData));

				// Debugging
				Serial.printlnf("Length = %d, Bytes left: %d", datalength, body->bytes_left);
				//Serial.println("Data:");
				//Serial.write(contentData, datalength);
				if (datalength <= 0)
				{
					// Problem here!!!
					ExitMyPage(cb, cbArg, result, "body->readerror");
					return;
				}
				// Here I need to check that I will read at least webkitformboundaryLength because
				// I dont want it partially in 2 buffers because the strstr wont find it.
				else if (body->bytes_left > 0 && body->bytes_left < webkitformboundaryLength)
				{
					// The ending seems to be splitted in 2 buffers...
					tempBufferLength = body->read(tempBuffer, sizeof(tempBuffer));
				}
				dataPos = 0;
			}

			// Debugging
			//Serial.println("Debug:");
			//Serial.write(contentData, datalength);

			// I need space in the contentData buffer.
			written = myFileWrite(destinationFile, &contentData[dataPos], datalength - webkitformboundaryLength);
			
			fileBytesReceived += written;
			if (written != datalength - webkitformboundaryLength)
			{
				//Serial.printlnf("Written: %d, Datalength: %d", written, datalength);
				ExitMyPage(cb, cbArg, result, "Write error 2");
				return;
			}

			// Move data
			for (int x = 0; x < webkitformboundaryLength; x++)
			{
				contentData[x] = contentData[datalength - webkitformboundaryLength + x];
			}


			if (tempBufferLength > 0)
			{
				Serial.println("Data is in partial buffer");
				// We have data in 2 partial buffers.
				// Add data in contentData
				memcpy(&contentData[webkitformboundaryLength], tempBuffer, tempBufferLength);
				datalength = webkitformboundaryLength + tempBufferLength;

			}
			else
				datalength = webkitformboundaryLength;
			
			// Search for the last expected line. Dont start at the beginning of the buffer because
			// strstr will stop at the first 0x00 byte.
			str = strstr((const char *)&contentData[datalength - webkitformboundaryLength], "------"); 
			if (str != NULL)
			{
				// last line is valid!
				dataPos = (str - (char*)contentData);
				//Serial.printlnf("DataPos of ending Webkit: %d", dataPos);

			}
			else
			{
				ExitMyPage(cb, cbArg, result, "Unexpected last line of data");
				return;
			}
			

			written = myFileWrite(destinationFile, &contentData[dataPos], datalength - webkitformboundaryLength);
			fileBytesReceived += written;
			if(written != datalength - webkitformboundaryLength)
			{
				ExitMyPage(cb, cbArg, result, "Write error 3");
				return;
			}

			myFileClose(destinationFile);

			Serial.printlnf("Total bytes received: %d", fileBytesReceived);
			if (fileBytesReceived < fileLength)
			{
				// We'll receive the file in more than one chunk.
				ExitMyPage(cb, cbArg, result, "Part Succeed!, waiting for next part");
			}
			else
			{
				Serial.printlnf("All data received. Device will upgrade.");
				upgradeDevice = 1;
			
				// Return http 200 and an html page to let the user know that the device will upgrade soon.
				ExitMyPage(cb, cbArg, result, "Succeed!");
				WiFi.listen(false); // We only need that if the SYSTEM_THREAD(ENABLED); is NOT enabled
				// because we wont execute the main loop
			}
			return;
		}
		else
		{
			Serial.println("Cannot find the beginning of the data");
			// Leave...
			cb(cbArg, 0, 404, nullptr, nullptr);
			return;
		}
    }

	int8_t idx = 0;
	for (;; idx++) {
		Page& p = myPages[idx];
		if (!p.url) {
			idx = -1;
			break;
		}
		else if (strcmp(url, p.url) == 0) {
			break;
		}
	}

	if (idx == -1) {
		cb(cbArg, 0, 404, nullptr, nullptr);
	}
	else {
		cb(cbArg, 0, 200, myPages[idx].mime_type, nullptr);
		result->write(myPages[idx].data);
	}
}

STARTUP(softap_set_application_page_handler(myPage, nullptr));

void handle_all_the_events(system_event_t event, int param)
{
	int ev = (int)event;
	if (ev != 4)
	{
		Serial.printlnf("Got event %d with value %d", ev, param);
	}
	if (ev == 1024)
		System.enableReset(); // allow reset!
	if (ev == 512)
		System.enableUpdates();
}
static unsigned long timer1Sec;
void setup()
{
#if CS_MEMORY == RGBR
	RGB.control(true);
	// Set rgb pins as output and left high...
	pinMode(CS_MEMORY, OUTPUT);
	digitalWrite(CS_MEMORY, HIGH);
	pinMode(RGBG, OUTPUT);
	digitalWrite(RGBG, HIGH);
	// on se sert pas du RBG blue
	pinMode(RGBB, OUTPUT);
	digitalWrite(RGBB, HIGH);

#endif
	System.on(all_events, handle_all_the_events);

	// Uncomment to wait for a byte to be received over the serial port to begin.
	Serial.begin(115200);
	while (!Serial.available()) Particle.process();
	Serial.printlnf("Hello SoftAP %s %s", __TIME__, __DATE__);
	if (myFileSystemInit())
	{
		Serial.println("Memory Init failed");
		while(1);
	}
	
	timer1Sec = millis();

	WiFi.listen();
	
}

void loop()
{
	// Check if mainloop is running while wifi.listen is called. Yes if SYSTEM_THREAD(ENABLED);
// 	if (timer1Sec + 1000 < millis())
// 	{
// 		Serial.println("Main loop");
// 		timer1Sec = millis();
// 	}

	//if (upgradeDevice)
	if(Serial.read() == 'u' || upgradeDevice)
	{
		Serial.println("Device will upgrade itself");
		delay(1000);

		uint16_t ff;
		uint8_t buffer[512];
		FileTransfer::Descriptor file;
		
		ff = myFileOpen(UPGRADE_FILE_NAME);
		Serial.printlnf("FileID: %d", ff);
		if (ff <= 0)
		{
			Serial.printlnf("Open error: %d", ff);
			delay(500);
			System.dfu();
			return;
		}
		uint32_t filesize = myFileGetSize(ff);
		
		Serial.printlnf("starting flash size=%d", filesize);

		file.file_length = filesize;
		file.file_address = 0; // Automatically set to HAL_OTA_FlashAddress if store is FIRMWARE
		file.chunk_address = 0;
		file.chunk_size = 512; // use default
		file.store = FileTransfer::Store::FIRMWARE;

		int result = Spark_Prepare_For_Firmware_Update(file, 0, NULL);
		if (result != 0) {
			Serial.printlnf("prepare failed %d", result);
			return;
		}
		file.chunk_address = file.file_address;

		int readLen = 0, totalRead = 0;
		do
		{
			int toRead = (totalRead + sizeof(buffer) < filesize) ? sizeof(buffer) : filesize - totalRead;
			readLen = myFileRead(ff, buffer, toRead);
			if (readLen == toRead)
			{
				totalRead += readLen;
				file.chunk_size = readLen;
				//Serial.write((const uint8_t *)buffer, readLen);
				result = Spark_Save_Firmware_Chunk(file, buffer, NULL);
				if (result != 0) {
					Serial.printlnf("save chunk failed %d", result);
					return;
				}
				file.chunk_address += file.chunk_size;
			}
			
		} while (readLen > 0 && totalRead < filesize);

		Serial.printlnf("TotalRead: %d", totalRead);

		myFileClose(ff);
		upgradeDevice = 0;
		result = Spark_Finish_Firmware_Update(file, 0x1, NULL);
		if (result != 0) {
			Serial.printlnf("Failed %d", result);
			return;
		}
		Serial.printlnf("Update completed");
	}
}