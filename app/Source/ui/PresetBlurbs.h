#pragma once

#include <juce_core/juce_core.h>

// One short plain sentence per bundled preset, shown under the preset selector. Presets that are
// not listed (a user's own, or a new bundled one) get a generic line.
namespace PresetBlurbs
{
    inline juce::String forPreset (const juce::String& presetName)
    {
        struct Entry { const char* name; const char* blurb; };
        static const Entry table[] = {
            { "Default",                      "Your own settings. Pick a preset to start from a ready-made sound." },
            { "Movies",                       "Clear dialogue, a wider stage and punchy explosions." },
            { "Music",                        "Fuller, clearer sound for everyday listening." },
            { "70's",                         "Warm and smooth, like a classic vinyl record." },
            { "80's",                         "Bright, shiny and full of that polished pop punch." },
            { "Alternative Rock",             "Crunchy guitars up front with a solid, driving low end." },
            { "Bass",                         "Heavy, deep low end that you can feel." },
            { "Bass - ambience",              "Deep bass wrapped in a roomy, spacious feel." },
            { "Classic Rock",                 "Big guitars and drums with plenty of body." },
            { "Classical",                    "Natural, open and detailed for orchestras and piano." },
            { "Conotating Life",              "A rich, wide sound with extra warmth and depth." },
            { "Jazz",                         "Warm and relaxed, with clear horns and soft cymbals." },
            { "Metal",                        "Tight, aggressive lows and sharp, cutting guitars." },
            { "Modern Country",               "Clear vocals and twang with a polished, full band." },
            { "Modern Rock",                  "Loud and punchy with thick guitars and strong drums." },
            { "Panserotaliya",                "An airy, spacious sound with a lifted top end." },
            { "Pop",                          "Crisp vocals and a tight beat that stands out." },
            { "Quizal Star",                  "Lively and bright with a wide, sparkling feel." },
            { "R&B",                          "Smooth vocals over a deep, rounded bass." },
            { "Trap",                         "Hard-hitting 808 bass and crisp, snappy hi-hats." },
            { "Underworld Life",              "A dark, deep and heavy sound with a lot of weight." },
            { "Yoznogarda dy flybe - Life",   "A big, dreamy sound with lots of space and shimmer." },
        };

        // "Bass (Quizal)" -> "Bass": the legend and the table both drop the author.
        const auto base = presetName.upToFirstOccurrenceOf (" (", false, false).trim();
        for (const auto& entry : table)
            if (base.equalsIgnoreCase (entry.name))
                return entry.blurb;

        return "Your own settings. Pick a preset to start from a ready-made sound.";
    }
}
