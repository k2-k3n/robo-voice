// Robo Voice: what the controllers show and the CC each control sends (channel 1).
// src/app.cpp binds the same numbers in kBind.
const NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const note = v => `${NOTES[v % 12]}${2 + Math.floor(v / 12)}`;

export const profile = {
  name: 'Robo Voice',
  accent: '#4fbf6a',
  pages: [
    {key: 'P', label: 'PLAY', title: 'Make it speak', color: '#82E882', controls: [
      {id: 'shout', cc: 30, kind: 'trigger', label: 'SHOUT', key: 'A'},
      {id: 'hiss', cc: 31, kind: 'trigger', label: 'HISS', key: 'S'},
      {id: 'chord', cc: 32, kind: 'trigger', label: 'CHORD', key: 'D'},
      {id: 'high', cc: 33, kind: 'trigger', label: 'HIGH', key: 'F'},
      {id: 'pitch', kind: 'xy', label: 'Note × Metal', x: 'note', y: 'metal'},
    ]},
    {key: 'V', label: 'VOICE', title: 'The voice', color: '#5FC8D8', controls: [
      {id: 'note', cc: 20, kind: 'range', label: 'Note', min: 0, max: 24, def: 9, format: note},
      {id: 'talk', cc: 21, kind: 'toggle', label: 'Talk', def: 1},
      {id: 'talkAmt', cc: 22, kind: 'range', label: 'Vowels', min: 0, max: 100, unit: '%', def: 85},
      {id: 'harmony', cc: 23, kind: 'choice', label: 'Harmony', options: ['Root', 'Octave', 'Power'], def: 1},
      {id: 'breath', cc: 24, kind: 'range', label: 'Breath', min: 0, max: 100, unit: '%', def: 30},
    ]},
    {key: 'C', label: 'CHIP', title: 'The chassis', color: '#F0A848', controls: [
      {id: 'metal', cc: 25, kind: 'range', label: 'Metal', min: 0, max: 100, def: 25,
        format: v => `${v < 34 ? 'Paint' : v < 67 ? 'Steel' : 'Chrome'} · ${v}`},
      {id: 'crush', cc: 26, kind: 'choice', label: 'Crush', options: ['Off', 'Lo-fi', 'Toy'], def: 0},
      {id: 'hum', cc: 27, kind: 'range', label: 'Hum', min: 0, max: 100, unit: '%', def: 60},
      {id: 'mix', cc: 28, kind: 'range', label: 'Mix', min: 0, max: 100, unit: '%', def: 88},
      {id: 'level', cc: 29, kind: 'range', label: 'Level', min: -12, max: 6, unit: 'dB', def: 0},
    ]},
  ],
};
