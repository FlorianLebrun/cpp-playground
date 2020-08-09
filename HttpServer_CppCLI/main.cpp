// Filename:  HttpServer.cs        
// Author:    Benjamin N. Summerton <define-private-public>        
// License:   Unlicense (http://unlicense.org/)

using namespace System;
using namespace System::IO;
using namespace System::Text;
using namespace System::Net;
using namespace System::Threading::Tasks;

ref class HttpServer
{
public:
  HttpListener^ listener;
  String^ url = "http://localhost:8000/";
  int pageViews = 0;
  int requestCount = 0;
  String^ pageData =
    "<!DOCTYPE>" +
    "<html>" +
    "  <head>" +
    "    <title>HttpListener Example</title>" +
    "  </head>" +
    "  <body>" +
    "    <p>Page Views: {0}</p>" +
    "    <form method=\"post\" action=\"shutdown\">" +
    "      <input type=\"submit\" value=\"Shutdown\" {1}>" +
    "    </form>" +
    "  </body>" +
    "</html>";


  Task HandleIncomingConnections()
  {
    bool runServer = true;

    // While a user hasn't visited the `shutdown` url, keep on handling requests
    while (runServer)
    {
      // Will wait here until we hear from a connection
      Task<HttpListenerContext^>^ ctx = this->listener->GetContextAsync();
      ctx->Run(() =>{

      })
      // Peel out the requests and response objects
      HttpListenerRequest req = ctx.Request;
      HttpListenerResponse resp = ctx.Response;

      // Print out some info about the request
      Console::WriteLine("Request #: {0}", ++requestCount);
      Console::WriteLine(req.Url.ToString());
      Console::WriteLine(req.HttpMethod);
      Console::WriteLine(req.UserHostName);
      Console::WriteLine(req.UserAgent);
      Console::WriteLine();

      // If `shutdown` url requested w/ POST, then shutdown the server after serving the page
      if ((req.HttpMethod == "POST") && (req.Url.AbsolutePath == "/shutdown"))
      {
        Console::WriteLine("Shutdown requested");
        runServer = false;
      }

      // Make sure we don't increment the page views counter if `favicon.ico` is requested
      if (req.Url.AbsolutePath != "/favicon.ico")
        pageViews += 1;

      // Write the response info
      string disableSubmit = !runServer ? "disabled" : "";
      byte[] data = Encoding.UTF8.GetBytes(String.Format(pageData, pageViews, disableSubmit));
      resp.ContentType = "text/html";
      resp.ContentEncoding = Encoding.UTF8;
      resp.ContentLength64 = data.LongLength;

      // Write out to the response stream (asynchronously), then close it
      await resp.OutputStream.WriteAsync(data, 0, data.Length);
      resp.Close();
    }
  }
  void main()
  {
    // Create a Http server and start listening for incoming connections
    listener = new HttpListener();
    listener.Prefixes.Add(url);
    listener.Start();
    Console::WriteLine("Listening for connections on {0}", url);

    // Handle requests
    Task listenTask = HandleIncomingConnections();
    listenTask.GetAwaiter().GetResult();

    // Close the listener
    listener.Close();
  }
}

void main()
{
  gcnew HttpServer().main();
}
