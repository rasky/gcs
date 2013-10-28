CXXFLAGS=-Wall -MMD -O3 
CFLAGS=-Wall -MMD -O3
all: gcs

-include *.d

GCS_OBJECTS = main.o gcs.o md5.o

gcs: $(GCS_OBJECTS)
	g++ $(LDFLAGS) $(GCS_OBJECTS) -o $@
clean:
	rm -f *~ *.o *.d gcs


