import string

NUMBER_OF_WORDS = 60

# each letter is numerically encoded with its position in the alphabet
# right padding to have same length as longest word

def phonol_reps (l1_input, l2_input, l1_output, l2_output):
	l1_inp = open(l1_input, "r")
	l2_inp = open(l2_input, "r")
	l1_out = open(l1_output, "w")
	l2_out = open(l2_output, "w")

	longest_l1 = 0
	longest_l2 = 0
	l1_phonol_reps = []
	l2_phonol_reps = []
	l1_phonol_words = []
	l2_phonol_words = []

	for _ in range (0, NUMBER_OF_WORDS): 
		l1_phonol = l1_inp.readline().split("\n", 2)[0].translate(None, "\"\'")
		l2_phonol = l2_inp.readline().split("\n", 2)[0].translate(None, "\"\'")

		l1_rep = []
		l2_rep = []
		for c in l1_phonol:
			l1_rep.append(ord(c))
		for c in l2_phonol:
			l2_rep.append(ord(c))

		l1_phonol_words.append(l1_phonol)
		l2_phonol_words.append(l2_phonol)
		l1_phonol_reps.append(l1_rep)
		l2_phonol_reps.append(l2_rep)

		if len(l1_phonol) > longest_l1:
			longest_l1 = len(l1_phonol)
		if len(l2_phonol) > longest_l2:
			longest_l2 = len(l2_phonol)
	print ("longest l1: {0} longest l2: {1}".format(longest_l1, longest_l2))
	
	for i in range(0, NUMBER_OF_WORDS):
		# add extra 0's here
		while len(l1_phonol_reps[i]) < longest_l1:
			l1_phonol_reps[i].append(0)
		l1_out.write("{0}\t{1}\n".format(l1_phonol_words[i], " ".join([str(x) for x in l1_phonol_reps[i]])))

		while len(l2_phonol_reps[i]) < longest_l2:
			l2_phonol_reps[i].append(0)
		l2_out.write("{0}\t{1}\n".format(l2_phonol_words[i], " ".join([str(x) for x in l2_phonol_reps[i]])))

	l1_inp.close()
	l2_inp.close()
	l1_out.close()
	l2_out.close()

if __name__ == "__main__":
	phonol_reps("es_sampa.txt", "en_sampa.txt", "l1_phonol.txt", "l2_phonol.txt")

