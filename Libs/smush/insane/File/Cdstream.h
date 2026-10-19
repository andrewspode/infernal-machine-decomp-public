#ifndef SMUSH_INSANE_CDSTREAM_H
#define SMUSH_INSANE_CDSTREAM_H

/*
 * The `STREAMER=insane_streamer` service Cdstream.c publishes (`streamer_interface`): the animation player (Pcplay.c) creates and
 * fills a stream through it, the frame stream (Framestream.c) advances it. `Advance` returns the frame's address, or a negative
 * code (-1 not ready, -2 frame too large), as an int.
 */
struct Stream;

typedef struct StreamerInterface
{
    int  (*Create)(struct Stream** ppStream, int ringSize, int frameSize);
    void (*Destroy)(struct Stream** ppStream);
    int  (*SetFile)(struct Stream* pStream, const char* pName, int startOffset);
    int  (*IsActive)(struct Stream* pStream);
    void (*Fill)(struct Stream* pStream, int limit);
    int  (*Advance)(struct Stream* pStream, int n);
    void (*Reset)(struct Stream* pStream);
    int  (*Percent)(struct Stream* pStream);
} StreamerInterface;

#endif
