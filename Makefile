PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CPPFLAGS += -I../public/utility/
LDFLAGS += -L../public/utility/ -lutility

OBJS = healthy_report_agent.o


all:	healthy_report_agent

healthy_report_agent:	$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr healthy_report_agent $(OBJS)
