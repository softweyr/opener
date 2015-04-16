// Raspberry Pi garage door button pusher
// Copyright 2015 Wes Peters
// See attached license for usage restrictions

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>

#include <iostream>
#include <locale>
#include <string>
#include <sstream>
#include <vector>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

class gpio
{
    public:
    gpio()
    {
        m_fd = open("/dev/gpioc0", O_RDWR);

        // Configure pins 20 and 21 for output
        // pin 20 is button "0",
        // pin 21 is button "1"

        struct gpio_pin pinStatus;

        pinStatus.gp_pin = 20;
        int r = ioctl(m_fd, GPIOGETCONFIG, &pinStatus);
        pinStatus.gp_flags = GPIO_PIN_OUTPUT;
        r = ioctl(m_fd, GPIOSETCONFIG, &pinStatus);

        pinStatus.gp_pin = 21;
        r = ioctl(m_fd, GPIOGETCONFIG, &pinStatus);
        pinStatus.gp_flags = GPIO_PIN_OUTPUT;
        r = ioctl(m_fd, GPIOSETCONFIG, &pinStatus);

        // Set initial state to high = relay off

        struct gpio_req rq;
        rq.gp_pin = 20;
        rq.gp_value = GPIO_PIN_HIGH;
        r = ioctl(m_fd, GPIOSET, &rq);
        rq.gp_pin = 21;
        r = ioctl(m_fd, GPIOSET, &rq);

        std::cout << "Relay configured on pins 20 and 21" << std::endl;
    }

    ~gpio()
    {
        // Make certain both relays are "off"

        struct gpio_req rq;
        rq.gp_pin = 20;
        rq.gp_value = GPIO_PIN_HIGH;
        int r = ioctl(m_fd, GPIOSET, &rq);
        rq.gp_pin = 21;
        r = ioctl(m_fd, GPIOSET, &rq);
        close(m_fd);

        std::cout << "Relays opened" << std::endl;
    }

    void toggle(unsigned button)
    {
        struct gpio_req rq;
        rq.gp_pin = 20 + button;
        int r = ioctl(m_fd, GPIOTOGGLE, &rq);

        // Mash the button for a quarter second

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 250000000;
        nanosleep(&ts, &ts);

        r = ioctl(m_fd, GPIOTOGGLE, &rq);
    }

private:
    int m_fd;
};


struct webSeperators : std::ctype<char>
{
    webSeperators() : std::ctype<char>(get_table()) {}
    static mask const * get_table()
    {
        static mask rc[table_size];
        rc['?'] = std::ctype_base::space;
        rc['='] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        return &rc[0];
    }
};

static gpio G;

class MyRequestHandler : public HTTPRequestHandler
{
public:
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
  {
    resp.setStatus(HTTPResponse::HTTP_OK);
    resp.setContentType("text/html");

    std::string uri(req.getURI());
    std::cout << "URI is " << uri << std::endl;
    std::istringstream S(uri);

    S.imbue(locale(S.getloc(), new webSeperators));

    // Assume a single button per request

    std::string page, name;
    unsigned button;
    S >> page >> name >> button;
    std::cout << "page " << page << " name " << name << " button " << button << std::endl;

    // Mush the button

    G.toggle(button);

    // Respond to the browser

    ostream& out = resp.send();
    out << "<h1>Hello world!</h1>"
        << "<p>Count: "  << ++count         << "</p>"
        << "<p>Host: "   << req.getHost()   << "</p>"
        << "<p>Method: " << req.getMethod() << "</p>"
        << "<p>URI: "    << req.getURI()    << "</p>"
        << "<p> page: "  << page            << "</p>"
        << "<p> name: "  << name            << "</p>"
        << "<p> button: "<< button          << "</p>";
    out.flush();

    cout << endl
         << "Response sent for count=" << count
         << " and URI=" << req.getURI() << endl;
  }

private:
  static int count;
};

int MyRequestHandler::count = 0;

class MyRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
  virtual HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &)
  {
    return new MyRequestHandler;
  }
};

class MyServerApp : public ServerApplication
{
protected:
  int main(const vector<string> &)
  {
    HTTPServer s(new MyRequestHandlerFactory, ServerSocket(9090), new HTTPServerParams);

    s.start();
    cout << endl << "Server started" << endl;

    waitForTerminationRequest();  // wait for CTRL-C or kill

    cout << endl << "Shutting down..." << endl;
    s.stop();

    return Application::EXIT_OK;
  }
};

int main(int argc, char** argv)
{
  MyServerApp app;
  return app.run(argc, argv);
}
