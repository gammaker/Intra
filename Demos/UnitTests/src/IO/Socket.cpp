#include "IO.h"

#if(!defined(INTRA_NO_CONCURRENCY) && !defined(INTRA_NO_NETWORKING))

#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/Synchronized.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

#include "IO/Socket.h"
#include "IO/SocketReader.h"
#include "IO/SocketWriter.h"
#include "IO/HtmlWriter.h"
#include "IO/Std.h"

#include "Range/Output/Inserter.h"

using namespace Intra;
using namespace IO;

void TestSocketIO(FormattedWriter& output)
{
	FatalErrorStatus status;
	output.PrintLine("Started TestSocketIO.");
	ServerSocket server(SocketType::TCP, 8080, 4, status);
	if(server == null) output.PrintLine("Couldn't start server on port 8080.");
	else output.PrintLine("Server started on port 8080.");
	INTRA_ASSERT(server != null);

	Synchronized<FormattedWriter> syncOutput(&output);

	AtomicBool error{false};
	output.PrintLine("Starting client thread...");
	Thread thread("TestSocketIO", [&]() {
		syncOutput->PrintLine("Started client thread.");
		FatalErrorStatus status2;
		StreamSocket client(SocketType::TCP, "localhost", 8080, status2);
		StringView str = "Hello";
		syncOutput->PrintLine("Client connected, sending data...");
		client.Send(str.Data(), str.Length(), status2);
		char strBuf[100];
		syncOutput->PrintLine("Data sent, waiting for input...");
		if(client.WaitForInput())
		{
			syncOutput->PrintLine("[Receiving...]");
			size_t len = client.Receive(strBuf, sizeof(strBuf), status2);
			str = StringView(strBuf, len);
			syncOutput->PrintLine("[Received] ", str);
			if(str != "World") error.Set(true);
		}
		else syncOutput->PrintLine("Error waiting!");

		SocketReader reader(Cpp::Move(client));
		StringView firstLine = reader.ReadLine(strBuf);
		syncOutput->PrintLine("ReadLine: ", firstLine);
		if(firstLine != "Writer: Hello, Reader!") error.Set(true);
		OutputArrayRange<char> strOut = Range::Take(strBuf, 50);
		ToString(strOut, reader.ByLine(Range::Drop(strBuf, 50)), "\", \"", "[\"", "\"]");
		StringView linesAsString = strOut.GetWrittenData();
		syncOutput->PrintLine("Remaining line range: ", linesAsString);
		if(linesAsString != "[\"Second line.\", \"Third line.\"]") error.Set(true);
	});
	syncOutput->PrintLine("Client thread started, acception connection...");
	StreamSocket connectedClient = server.Accept(status);
	char strBuf[100];
	size_t len = connectedClient.Receive(strBuf, sizeof(strBuf), status);
	StringView str(strBuf, len);
	syncOutput->PrintLine(str);
	INTRA_ASSERT_EQUALS(str, "Hello");
	connectedClient.Send("World", 5, status);
	syncOutput->PrintLine("[Sent] World");

	SocketWriter writer(Cpp::Move(connectedClient));
	writer.PrintLine("Writer: Hello, Reader!")
		.PrintLine("Second line.")
		.PrintLine("Third line.");

	INTRA_ASSERT(!error.Get());
}

void TestHttpServer(FormattedWriter& output)
{
	FatalErrorStatus status;
	ServerSocket server(SocketType::TCP, 8080, 40, status);
	char requestBuf[1000];
	for(size_t counter=0; ; counter++)
	{
		StreamSocket connectedClient = server.Accept(status);
		size_t len = connectedClient.Receive(requestBuf, sizeof(requestBuf), status);
		StringView request(requestBuf, len);

		output.PrintLine("Processing request: ")
			.PrintLine(request);

		SocketWriter response(Cpp::Move(connectedClient));

		String str;
		HtmlWriter(LastAppender(str))
			.PushFont({0, 0, 0.75f}, 4, true)
			.PrintPreformatted(request)
			.PopFont()
			.PushFont({0.8f, 0, 0.5f}, 6, true)
			.PrintLine(counter)
			.PopFont();

		response.PrintLine("HTTP/1.1 200 OK")
			.PrintLine("Content-Type: text/html")
			.PrintLine("Content-Length: ", str.Length())
			.PrintLine()
			.PrintLine()
			.Print(str);
	}
}

#endif

#endif
