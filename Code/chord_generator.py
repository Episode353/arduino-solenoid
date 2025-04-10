
# Online# Chromatic scale for linear indexing
CHROMATIC_SCALE = [
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
]

# Enharmonic mapping for note normalization
ENHARMONICS = {
    "A#": "A#", "Bb": "A#",
    "B#": "C", "Cb": "B",
    "C#": "C#", "Db": "C#",
    "D#": "D#", "Eb": "D#",
    "E#": "F", "Fb": "E",
    "F#": "F#", "Gb": "F#",
    "G#": "G#", "Ab": "G#"
}

# Scale intervals for different modes
SCALE_INTERVALS = {
    "Major": [2, 2, 1, 2, 2, 2, 1],
    "Minor": [2, 1, 2, 2, 1, 2, 2],
    "Dorian": [2, 1, 2, 2, 2, 1, 2],
    "Phrygian": [1, 2, 2, 2, 1, 2, 2],
    "Lydian": [2, 2, 2, 1, 2, 2, 1],
    "Mixolydian": [2, 2, 1, 2, 2, 1, 2],
    "Locrian": [1, 2, 2, 1, 2, 2, 2],
    "Harmonic Minor": [2, 1, 2, 2, 1, 3, 1]
}

# Complexity mappings for chords
COMPLEXITY_CHORDS = {
    1: [0, 2, 4],  # Root, Third, Fifth
    2: [0, 2, 4, 5],
    3: [0, 2, 4, 6],
    4: [0, 3, 4, 6],
    5: [0, 3, 4],
    6: [0, 2, 5, 6],
    7: [0, 2, 4, 6, 8],
    8: [0, 2, 4, 6, 5],
    9: [0, 2, 4, 6, 9],
    10: [0, 2, 4, 6, 5, 10]
}

SCALES = {
    "Major": {
        "C": ["C", "D", "E", "F", "G", "A", "B"],
        "C#": ["C#", "D#", "E#", "F#", "G#", "A#", "B#"],
        "Db": ["Db", "Eb", "F", "Gb", "Ab", "Bb", "C"],
        "D": ["D", "E", "F#", "G", "A", "B", "C#"],
        "D#": ["D#", "E#", "F##", "G#", "A#", "B#", "C##"],
        "Eb": ["Eb", "F", "G", "Ab", "Bb", "C", "D"],
        "E": ["E", "F#", "G#", "A", "B", "C#", "D#"],
        "Fb": ["Fb", "Gb", "Ab", "Bbb", "Cb", "Db", "Eb"],
        "F": ["F", "G", "A", "Bb", "C", "D", "E"],
        "F#": ["F#", "G#", "A#", "B", "C#", "D#", "E#"],
        "Gb": ["Gb", "Ab", "Bb", "Cb", "Db", "Eb", "F"],
        "G": ["G", "A", "B", "C", "D", "E", "F#"],
        "Ab": ["Ab", "Bb", "C", "Db", "Eb", "F", "G"],
        "A": ["A", "B", "C#", "D", "E", "F#", "G#"],
        "A#": ["A#", "B#", "C##", "D#", "E#", "F##", "G##"],
        "Bb": ["Bb", "C", "D", "Eb", "F", "G", "A"],
        "B": ["B", "C#", "D#", "E", "F#", "G#", "A#"],
        "Cb": ["Cb", "Db", "Eb", "Fb", "Gb", "Ab", "Bb"],
    },
    "Minor": {
        "C": ["C", "D", "Eb", "F", "G", "Ab", "Bb"],
        "C#": ["C#", "D#", "E", "F#", "G#", "A", "B"],
        "Db": ["Db", "Eb", "Fb", "Gb", "Ab", "Bbb", "Cb"],
        "D": ["D", "E", "F", "G", "A", "Bb", "C"],
        "D#": ["D#", "E#", "F#", "G#", "A#", "B", "C#"],
        "Eb": ["Eb", "F", "Gb", "Ab", "Bb", "Cb", "Db"],
        "E": ["E", "F#", "G", "A", "B", "C", "D"],
        "Fb": ["Fb", "Gb", "Abb", "Bbb", "Cb", "Dbb", "Ebb"],
        "F": ["F", "G", "Ab", "Bb", "C", "Db", "Eb"],
        "F#": ["F#", "G#", "A", "B", "C#", "D", "E"],
        "Gb": ["Gb", "Ab", "Bbb", "Cb", "Db", "Ebb", "Fb"],
        "G": ["G", "A", "Bb", "C", "D", "Eb", "F"],
        "Ab": ["Ab", "Bb", "Cb", "Db", "Eb", "Fb", "Gb"],
        "A": ["A", "B", "C", "D", "E", "F", "G"],
        "A#": ["A#", "C", "D", "D#", "F", "G", "A"],
        "Bb": ["Bb", "C", "Db", "Eb", "F", "Gb", "Ab"],
        "B": ["B", "C#", "D", "E", "F#", "G", "A"],
        "Cb": ["Cb", "Db", "Ebb", "Fb", "Gb", "Abb", "Bbb"],
    },
 "Dorian": {
        "C": ["C", "D", "Eb", "F", "G", "A", "Bb"],
        "C#": ["C#", "D#", "E", "F#", "G#", "A#", "B"],
        "Db": ["Db", "Eb", "Fb", "Gb", "Ab", "Bb", "Cb"],
        "D": ["D", "E", "F", "G", "A", "B", "C"],
        "D#": ["D#", "E#", "F#", "G#", "A#", "B#", "C#"],
        "Eb": ["Eb", "F", "Gb", "Ab", "Bb", "C", "Db"],
        "E": ["E", "F#", "G", "A", "B", "C#", "D"],
        "Fb": ["Fb", "Gb", "Abb", "Bbb", "Cb", "Db", "Ebb"],
        "F": ["F", "G", "Ab", "Bb", "C", "D", "Eb"],
        "F#": ["F#", "G#", "A", "B", "C#", "D#", "E"],
        "Gb": ["Gb", "Ab", "Bbb", "Cb", "Db", "Eb", "Fb"],
        "G": ["G", "A", "Bb", "C", "D", "E", "F"],
        "G#": ["G#", "A#", "B", "C#", "D#", "E#", "F#"],
        "Ab": ["Ab", "Bb", "Cb", "Db", "Eb", "F", "Gb"],
        "A": ["A", "B", "C", "D", "E", "F#", "G"],
        "A#": ["A#", "B#", "C#", "D#", "E#", "F##", "G#"],
        "Bb": ["Bb", "C", "Db", "Eb", "F", "G", "Ab"],
        "B": ["B", "C#", "D", "E", "F#", "G#", "A"],
        "Cb": ["Cb", "Db", "Ebb", "Fb", "Gb", "Ab", "Bbb"],
    },
       "Phrygian": {
        "C": ["C", "Db", "Eb", "F", "G", "Ab", "Bb"],
        "C#": ["C#", "D", "E", "F#", "G#", "A", "B"],
        "Db": ["Db", "Ebb", "Fb", "Gb", "Ab", "Bbb", "Cb"],
        "D": ["D", "Eb", "F", "G", "A", "Bb", "C"],
        "D#": ["D#", "E", "F#", "G#", "A#", "B", "C#"],
        "Eb": ["Eb", "Fb", "Gb", "Ab", "Bb", "Cb", "Db"],
        "E": ["E", "F", "G", "A", "B", "C", "D"],
        "Fb": ["Fb", "Gbb", "Abb", "Bbb", "Cb", "Dbb", "Ebb"],
        "F": ["F", "Gb", "Ab", "Bb", "C", "Db", "Eb"],
        "F#": ["F#", "G", "A", "B", "C#", "D", "E"],
        "Gb": ["Gb", "Abb", "Bbb", "Cb", "Db", "Ebb", "Fb"],
        "G": ["G", "Ab", "Bb", "C", "D", "Eb", "F"],
        "G#": ["G#", "A", "B", "C#", "D#", "E", "F#"],
        "Ab": ["Ab", "Bbb", "Cb", "Db", "Eb", "Fb", "Gb"],
        "A": ["A", "Bb", "C", "D", "E", "F", "G"],
        "A#": ["A#", "B", "C#", "D#", "E#", "F#", "G#"],
        "Bb": ["Bb", "Cb", "Db", "Eb", "F", "Gb", "Ab"],
        "B": ["B", "C", "D", "E", "F#", "G", "A"],
        "Cb": ["Cb", "Dbb", "Ebb", "Fb", "Gb", "Abb", "Bbb"],
    },
    "Lydian": {
        "C": ["C", "D", "E", "F#", "G", "A", "B"],
        "C#": ["C#", "D#", "E#", "F##", "G#", "A#", "B#"],
        "Db": ["Db", "Eb", "F", "G", "Ab", "Bb", "C"],
        "D": ["D", "E", "F#", "G#", "A", "B", "C#"],
        "D#": ["D#", "E#", "F##", "G##", "A#", "B#", "C##"],
        "Eb": ["Eb", "F", "G", "A", "Bb", "C", "D"],
        "E": ["E", "F#", "G#", "A#", "B", "C#", "D#"],
        "Fb": ["Fb", "Gb", "Ab", "Bb", "Cb", "Db", "Eb"],
        "F": ["F", "G", "A", "B", "C", "D", "E"],
        "F#": ["F#", "G#", "A#", "B#", "C#", "D#", "E#"],
        "Gb": ["Gb", "Ab", "Bb", "Cb", "Db", "Eb", "F"],
        "G": ["G", "A", "B", "C#", "D", "E", "F#"],
        "G#": ["G#", "A#", "B#", "C##", "D#", "E#", "F##"],
        "Ab": ["Ab", "Bb", "C", "D", "Eb", "F", "G"],
        "A": ["A", "B", "C#", "D#", "E", "F#", "G#"],
        "A#": ["A#", "B#", "C##", "D##", "E#", "F##", "G##"],
        "Bb": ["Bb", "C", "D", "E", "F", "G", "A"],
        "B": ["B", "C#", "D#", "E#", "F#", "G#", "A#"],
        "Cb": ["Cb", "Db", "Eb", "Fb", "Gb", "Ab", "Bb"],
    },
 "Mixolydian": {
        "C": ["C", "D", "E", "F", "G", "A", "Bb"],
        "C#": ["C#", "D#", "E#", "F#", "G#", "A#", "B"],
        "Db": ["Db", "Eb", "F", "Gb", "Ab", "Bb", "Cb"],
        "D": ["D", "E", "F#", "G", "A", "B", "C"],
        "D#": ["D#", "E#", "F##", "G#", "A#", "B#", "C#"],
        "Eb": ["Eb", "F", "G", "Ab", "Bb", "C", "Db"],
        "E": ["E", "F#", "G#", "A", "B", "C#", "D"],
        "F": ["F", "G", "A", "Bb", "C", "D", "Eb"],
        "F#": ["F#", "G#", "A#", "B", "C#", "D#", "E"],
        "Gb": ["Gb", "Ab", "Bb", "Cb", "Db", "Eb", "Fb"],
        "G": ["G", "A", "B", "C", "D", "E", "F"],
        "G#": ["G#", "A#", "B#", "C#", "D#", "E#", "F#"],
        "Ab": ["Ab", "Bb", "C", "Db", "Eb", "F", "Gb"],
        "A": ["A", "B", "C#", "D", "E", "F#", "G"],
        "A#": ["A#", "B#", "C##", "D#", "E#", "F##", "G#"],
        "Bb": ["Bb", "C", "D", "Eb", "F", "G", "Ab"],
        "B": ["B", "C#", "D#", "E", "F#", "G#", "A"],
        "Cb": ["Cb", "Db", "Eb", "Fb", "Gb", "Ab", "Bbb"],
    },
 "Locrian": {
        "C": ["C", "Db", "Eb", "F", "Gb", "Ab", "Bb"],
        "C#": ["C#", "D", "E", "F#", "G", "A", "B"],
        "Db": ["Db", "Ebb", "Fb", "Gb", "Abb", "Bbb", "Cb"],
        "D": ["D", "Eb", "F", "G", "Ab", "Bb", "C"],
        "D#": ["D#", "E", "F#", "G#", "A", "B", "C#"],
        "Eb": ["Eb", "Fb", "Gb", "Ab", "Bbb", "Cb", "Db"],
        "E": ["E", "F", "G", "A", "Bb", "C", "D"],
        "Fb": ["Fb", "Gbb", "Abb", "Bbb", "Cb", "Dbb", "Ebb"],
        "F": ["F", "Gb", "Ab", "Bb", "Cb", "Db", "Eb"],
        "F#": ["F#", "G", "A", "B", "C", "D", "E"],
        "Gb": ["Gb", "Abb", "Bbb", "Cb", "Dbb", "Ebb", "Fb"],
        "G": ["G", "Ab", "Bb", "C", "Db", "Eb", "F"],
        "G#": ["G#", "A", "B", "C#", "D", "E", "F#"],
        "Ab": ["Ab", "Bbb", "Cb", "Db", "Ebb", "Fb", "Gb"],
        "A": ["A", "Bb", "C", "D", "Eb", "F", "G"],
        "A#": ["A#", "B", "C#", "D#", "E", "F#", "G#"],
        "Bb": ["Bb", "Cb", "Db", "Eb", "Fb", "Gb", "Ab"],
        "B": ["B", "C", "D", "E", "F", "G", "A"],
        "Cb": ["Cb", "Dbb", "Ebb", "Fb", "Gb", "Abb", "Bbb"],
    },
 "Harmonic Minor": {
        "C": ["C", "D", "Eb", "F", "G", "Ab", "B"],
        "C#": ["C#", "D#", "E", "F#", "G#", "A", "B#"],
        "Db": ["Db", "Eb", "Fb", "Gb", "Ab", "Bbb", "C"],
        "D": ["D", "E", "F", "G", "A", "Bb", "C#"],
        "D#": ["D#", "E#", "F#", "G#", "A#", "B", "C##"],
        "Eb": ["Eb", "F", "Gb", "Ab", "Bb", "Cb", "D"],
        "E": ["E", "F#", "G", "A", "B", "C", "D#"],
        "Fb": ["Fb", "Gb", "Abb", "Bbb", "Cb", "Dbb", "Ebb"],
        "F": ["F", "G", "Ab", "Bb", "C", "Db", "E"],
        "F#": ["F#", "G#", "A", "B", "C#", "D", "E#"],
        "Gb": ["Gb", "Ab", "Bbb", "Cb", "Db", "Ebb", "F"],
        "G": ["G", "A", "Bb", "C", "D", "Eb", "F#"],
        "G#": ["G#", "A#", "B", "C#", "D#", "E", "F##"],
        "Ab": ["Ab", "Bb", "Cb", "Db", "Eb", "Fb", "G"],
        "A": ["A", "B", "C", "D", "E", "F", "G#"],
        "A#": ["A#", "B#", "C#", "D#", "E#", "F#", "G##"],
        "Bb": ["Bb", "C", "Db", "Eb", "F", "Gb", "A"],
        "B": ["B", "C#", "D", "E", "F#", "G", "A#"],
        "Cb": ["Cb", "Db", "Ebb", "Fb", "Gb", "Abb", "Bb"],
    },
}


def normalize_note(note):
    """Convert enharmonic notes to match the chromatic scale."""
    note = note.capitalize()
    return ENHARMONICS.get(note, note)

# Generate scales dynamically
def generate_scale(tonic, scale_type):
    """Retrieve a predefined scale."""
    # Normalize the tonic for enharmonic mapping
    tonic = normalize_note(tonic)

    # Ensure scale type exists
    if scale_type not in SCALES:
        raise ValueError(f"Scale type '{scale_type}' is not defined.")

    # Fetch the predefined scale for the tonic
    scale_dict = SCALES[scale_type]
    if tonic not in scale_dict:
        raise ValueError(f"Tonic '{tonic}' is not defined in {scale_type} scale.")

    # Return the predefined scale
    return scale_dict[tonic]



# Compare scales with enharmonic equivalence
def compare_scales(expected, generated):
    """Compare scales, accounting for enharmonic equivalence."""
    return all(normalize_note(e) == normalize_note(g) for e, g in zip(expected, generated))

# Verify scales internally
def verify_scale_internal(scale_type, tonic):
    """Verify a scale against predefined scales for the given tonic and scale type."""
    intervals = SCALE_INTERVALS.get(scale_type)
    if not intervals:
        raise ValueError(f"Scale type '{scale_type}' is not defined.")
    if tonic not in SCALES[scale_type]:
        raise ValueError(f"Tonic '{tonic}' is not defined in scale type '{scale_type}'.")
    expected_notes = SCALES[scale_type][tonic]
    generated_notes = generate_scale(tonic, intervals)
    if not compare_scales(expected_notes, generated_notes):
        raise ValueError(
            f"Verification failed for {scale_type} scale with tonic {tonic}.\n"
            f"Expected: {expected_notes}, Generated: {generated_notes}"
        )

def build_chord(scale_type, tonic, numeral_index, complexity, debug=False):
    """Build a chord based on the specified scale type and complexity."""
    # Generate the scale for the specified type
    scale_notes = generate_scale(tonic, scale_type)
    if debug:
        print(f"Scale Notes: {scale_notes}")  # Debug print

    # Ensure the numeral_index is valid
    if not (0 <= numeral_index < len(scale_notes)):
        raise ValueError(f"Numeral index {numeral_index} is out of bounds for scale of length {len(scale_notes)}.")

    # Calculate chord indices
    chord_indices = []
    scale_length = len(scale_notes)
    for offset in complexity:
        # Initial index with wrapping
        index = (numeral_index + offset) % scale_length

        # Handle offsets larger than the scale length to avoid duplicates
        while offset >= scale_length:
            offset -= scale_length  # Adjust the offset for larger cycles
            index += 1  # Move to the next sequential note
            index %= scale_length  # Wrap around if necessary

        chord_indices.append(index)

    if debug:
        print(f"Chord Indices: {chord_indices}")  # Debug

    # Fetch the chord notes in order, allowing duplicates in indices
    chord = [scale_notes[i] for i in chord_indices]

    # Preserve all notes as generated without deduplication
    final_chord = chord[:]

    if debug:
        print(f"Final Chord: {final_chord}")  # Debug
    return final_chord



def test_chord_transformation():
    """Test chord generation with known inputs and expected outputs."""
    tests = [
        {
            "tonic": "G",
            "scale_type": "Mixolydian",
            "numeral": "VII",
            "complexity": 9,
            "expected_chord": ["F", "A", "C", "E", "B"]
        },
        {
            "tonic": "C",
            "scale_type": "Major",
            "numeral": "I",
            "complexity": 9,
            "expected_chord": ["C", "E", "G", "B", "F"]
        }
    ]

    for test in tests:
        print(f"\nTesting {test['tonic']} {test['scale_type']} Numeral {test['numeral']} Complexity {test['complexity']}")
        numeral_index = ["I", "II", "III", "IV", "V", "VI", "VII"].index(test["numeral"])
        complexity_offsets = COMPLEXITY_CHORDS[test["complexity"]]
        result = build_chord(test["scale_type"], test["tonic"], numeral_index, complexity_offsets)
        if result == test["expected_chord"]:
            print(f"PASS: {result}")
        else:
            print(f"FAIL: Expected {test['expected_chord']}, but got {result}")


# Main function
# Other code defining constants, normalize_note, generate_scale, etc.

def invert_chord(chord, inversion):
    """
    Perform a specified inversion on a chord.

    :param chord: List of notes in the chord.
    :param inversion: Inversion number to apply (1-6).
    :return: List of notes after applying the specified inversion.
    """
    if not chord:
        return []

    num_notes = len(chord)
    effective_inversion = inversion % num_notes  # Wrap inversion number
    return chord[effective_inversion:] + chord[:effective_inversion]


def build_chord_with_inversion(scale_type, tonic, numeral_index, complexity, inversion, debug=False):
    """
    Build a chord based on the specified scale type, complexity, and inversion.

    :param scale_type: The scale type (e.g., "Major", "Minor").
    :param tonic: The root note of the scale.
    :param numeral_index: The scale degree (index of the numeral in the scale).
    :param complexity: Complexity mapping for chord construction.
    :param inversion: Inversion number to apply to the chord.
    :param debug: Debug flag for additional output.
    :return: List of notes in the chord after inversion.
    """
    # Generate the chord using the base logic
    chord = build_chord(scale_type, tonic, numeral_index, complexity, debug)

    # Apply inversion
    inverted_chord = invert_chord(chord, inversion)

    if debug:
        print(f"Original Chord: {chord}")
        print(f"Inverted Chord (Inversion {inversion}): {inverted_chord}")

    return inverted_chord


def main():
    """Interactive mode for chord generation with inversion support."""
    letter = input("Enter the Letter (e.g., C, G, A#): ").strip()
    key = input("Enter the Key (Major, Minor, etc.): ").strip().capitalize()
    complexity = input("Enter the Complexity (1-10): ").strip()
    numeral = input("Enter the Numeral (I, II, III, IV, V, VI, VII): ").strip().upper()
    inversion = input("Enter the Inversion (1-6): ").strip()

    letter = normalize_note(letter)

    if key not in SCALE_INTERVALS:
        print(f"Invalid key: {key}. Available keys: {', '.join(SCALE_INTERVALS.keys())}")
        return
    if numeral not in ["I", "II", "III", "IV", "V", "VI", "VII"]:
        print(f"Invalid numeral: {numeral}. Available numerals: I, II, III, IV, V, VI, VII")
        return
    if not complexity.isdigit() or int(complexity) not in COMPLEXITY_CHORDS:
        print(f"Invalid complexity: {complexity}. Must be an integer between 1 and 10.")
        return
    if not inversion.isdigit() or int(inversion) < 1:
        print(f"Invalid inversion: {inversion}. Must be an integer greater than or equal to 1.")
        return

    complexity = int(complexity)
    numeral_index = ["I", "II", "III", "IV", "V", "VI", "VII"].index(numeral)
    inversion = int(inversion)

    try:
        chord = build_chord_with_inversion(key, letter, numeral_index, COMPLEXITY_CHORDS[complexity], inversion)
        print(f"Generated chord with inversion {inversion}: {' '.join(chord)}")
    except ValueError as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()  # Run the interactive mode
    print("\nRunning Validation Tests...\n")
    test_chord_transformation()