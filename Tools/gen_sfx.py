"""Procedurally synthesize Runik's sound effects and ambient music as 16-bit
mono WAVs into Content/Audio/. Pure python (no numpy required)."""
import math
import os
import random
import struct
import wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Content", "Audio")
SR = 44100

random.seed(1337)


def write_wav(name, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    norm = 0.85 / peak if peak > 0.85 else 1.0
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(
            struct.pack("<h", int(max(-1, min(1, s * norm)) * 32767))
            for s in samples))
    print("wrote", name)


def env(i, n, attack=0.01, release=0.3):
    """Simple attack/release envelope, t in samples."""
    t = i / SR
    dur = n / SR
    a = min(1.0, t / max(1e-5, attack))
    r = min(1.0, (dur - t) / max(1e-5, release))
    return max(0.0, min(a, r))


def tone(freq, dur, vol=0.5, partials=((1, 1.0),), attack=0.01, release=None,
         vibrato=0.0, vib_rate=5.0, decay=3.0):
    n = int(SR * dur)
    release = release if release is not None else dur * 0.6
    out = []
    for i in range(n):
        t = i / SR
        f = freq * (1.0 + vibrato * math.sin(2 * math.pi * vib_rate * t))
        s = 0.0
        for mult, amp in partials:
            s += amp * math.sin(2 * math.pi * f * mult * t) * math.exp(-decay * t * mult ** 0.5)
        out.append(vol * s * env(i, n, attack, release))
    return out


def noise_sweep(dur, f0, f1, vol=0.5, attack=0.02, release=None):
    """Filtered noise whoosh: one-pole lowpass with sweeping cutoff."""
    n = int(SR * dur)
    release = release if release is not None else dur * 0.5
    out = []
    lp = 0.0
    for i in range(n):
        t = i / n
        cutoff = f0 + (f1 - f0) * t
        alpha = min(0.99, 2 * math.pi * cutoff / SR)
        lp += alpha * (random.uniform(-1, 1) - lp)
        out.append(vol * lp * env(i, n, attack, release))
    return out


def mix(*layers):
    n = max(len(l) for l in layers)
    out = [0.0] * n
    for l in layers:
        for i, s in enumerate(l):
            out[i] += s
    return out


def delay_into(samples, offset_s):
    return [0.0] * int(SR * offset_s) + samples


CHIME = ((1, 1.0), (2.76, 0.45), (5.4, 0.22), (8.9, 0.1))  # bell-like partials


def main():
    os.makedirs(OUT, exist_ok=True)

    # UI
    write_wav("sfx_click.wav", tone(1900, 0.06, 0.4, ((1, 1), (2, .3)), decay=28))
    write_wav("sfx_hover.wav", tone(1200, 0.05, 0.18, ((1, 1),), decay=30))
    write_wav("sfx_error.wav", mix(tone(160, 0.25, 0.5, ((1, 1), (1.5, .6)), decay=6),
                                   tone(110, 0.25, 0.4, ((1, 1),), decay=6)))
    write_wav("sfx_tick.wav", tone(2600, 0.03, 0.22, ((1, 1),), decay=40))

    # placing / inscribing runes
    write_wav("sfx_place.wav", mix(tone(90, 0.22, 0.9, ((1, 1), (2, .4)), decay=14),
                                   noise_sweep(0.08, 4000, 500, 0.35)))
    scratch = []
    for k in range(4):
        seg = noise_sweep(0.11, 1500 + 700 * k, 2600 + 500 * k, 0.3, attack=0.03)
        scratch = mix(scratch, delay_into(seg, 0.09 * k)) if scratch else seg
    write_wav("sfx_inscribe.wav", scratch)

    # rune actions
    write_wav("sfx_push.wav", noise_sweep(0.35, 400, 3200, 0.7))
    write_wav("sfx_pull.wav", noise_sweep(0.35, 3200, 400, 0.7))
    write_wav("sfx_orbit.wav", tone(520, 0.4, 0.4, CHIME, vibrato=0.02, vib_rate=9, decay=5))
    write_wav("sfx_fuse.wav", mix(tone(660, 0.9, 0.5, CHIME, decay=4),
                                  delay_into(tone(990, 0.7, 0.4, CHIME, decay=4), 0.06),
                                  noise_sweep(0.12, 5000, 800, 0.2)))
    write_wav("sfx_detach.wav", mix(noise_sweep(0.07, 6000, 2000, 0.6),
                                    delay_into(tone(880, 0.4, 0.35, CHIME, decay=7), 0.03),
                                    delay_into(tone(587, 0.5, 0.3, CHIME, decay=6), 0.12)))
    shimmer = []
    for k, f in enumerate((392, 494, 587, 784)):
        shimmer = mix(shimmer, delay_into(
            tone(f, 0.8, 0.28, CHIME, vibrato=0.015, vib_rate=6, decay=3.5), 0.05 * k)) \
            if shimmer else tone(f, 0.8, 0.28, CHIME, vibrato=0.015, vib_rate=6, decay=3.5)
    write_wav("sfx_shift.wav", shimmer)

    # outcome
    fanfare = []
    for k, f in enumerate((523, 659, 784, 1047, 1319)):
        seg = tone(f, 0.9, 0.35, CHIME, decay=3)
        fanfare = mix(fanfare, delay_into(seg, 0.12 * k)) if fanfare else seg
    write_wav("sfx_win.wav", fanfare)
    write_wav("sfx_output.wav", mix(tone(1047, 0.5, 0.4, CHIME, decay=5),
                                    delay_into(tone(1568, 0.4, 0.3, CHIME, decay=6), 0.05)))

    # ambient beds (loopable-ish long fades)
    def drone(base, dur, name):
        n = int(SR * dur)
        out = []
        for i in range(n):
            t = i / SR
            s = 0.0
            for mult, amp in ((1, .5), (1.5, .2), (2.0, .18), (3.01, .08)):
                lfo = 1 + 0.008 * math.sin(2 * math.pi * (0.11 * mult) * t + mult)
                s += amp * math.sin(2 * math.pi * base * mult * lfo * t)
            # slow swell so loop seam is quiet
            fade = min(1.0, t / 3.0, (dur - t) / 3.0)
            out.append(0.5 * s * fade)
        write_wav(name, out)

    drone(55.0, 24.0, "amb_menu.wav")
    drone(73.4, 24.0, "amb_forge.wav")

    print("done")


if __name__ == "__main__":
    main()
