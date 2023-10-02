#ifndef CHANNEL
#define CHANNEL

typedef struct {
} Channel;

int channel_init(Channel* chan);

int destroy_channel(Channel* chan);

void* channel_read_from(Channel* chan);

int channel_write_to(Channel* chan, void* data);

#endif
