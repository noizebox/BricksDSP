import math, wave, struct

# No of samples in the lowest octave
INITIAL_TABLE_LENGTH = 8192
SAMPLES_PER_LINE = 10

SAW = 0
SQUARE = 1
TRIANGLE = 2
SINE = 3

waveform = SQUARE

global samplecount
samplecount = 0
MAX_INT = math.pow(2, 15) - 1

def add_harmonic(buffer, harmonic, level):
	return [s + 0.5 * level * math.sin(harmonic * 2 * math.pi * i / len(buffer)) for i,s in enumerate(buffer)]

def write_wav(file, buffer):
	for s in buffer:
		value = int(32767.0 * s)
		data = struct.pack('<h', value)
		file.writeframesraw(data)

def write_wt(file, buffer):
	global samplecount
	for sample in buffer:
		file.write(str(round(sample, 7)) + ", ")
		samplecount += 1
		if samplecount >= SAMPLES_PER_LINE:
			samplecount = 0
			file.write("\n")


def main():
	wav_file = wave.open("waveform.wav", 'w')
	wav_file.setnchannels(1)
	wav_file.setframerate(44100)
	wav_file.setsampwidth(2)

	wt_file = open("wavetable.txt", "w")
	wt_file.write("constexpr float wavetable[] = {")
	table_len = INITIAL_TABLE_LENGTH
	harmonics = 1000.0

	for oct in range (0, 10):

		buffer = [0 for x in range(0,table_len)]
		for h in range (1, int(round(harmonics, 0))):
			if waveform == SAW:
				buffer = add_harmonic(buffer, h, 1.0/float(h))

			if waveform == SQUARE:
				if h % 2 == 1:
					buffer = add_harmonic(buffer, h, 2.0/float(h))

			if waveform == TRIANGLE:
				if h % 4 == 1:
					buffer = add_harmonic(buffer, h, 1.6/(float(h) * float(h)))

				elif h % 4 == 3:
					buffer = add_harmonic(buffer, h, -1.6/(float(h) * float(h)))

			if waveform == SINE:
				if h == 1:
					buffer = add_harmonic(buffer, h, 1.0/float(h))
	
		write_wav(wav_file, buffer)
		write_wt(wt_file, buffer)

		table_len = table_len / 2
		harmonics = harmonics / 2

	wav_file.close
	wt_file.write ("0.0 };") 
	wt_file.close
if __name__ == "__main__":	
	main()

