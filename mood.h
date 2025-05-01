#ifndef MOOD_H
#define MOOD_H

typedef enum {
    NEUTRAL,
    SARCASTIC,
    ROMANTIC
} Mood;

extern Mood current_mood;

#endif