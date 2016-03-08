import string

NUMBER_OF_WORDS = 60

# each letter is numerically encoded with its position in the alphabet
# right padding to have same length as longest word

def lex_reps (l1_input, l2_input, l1_output, l2_output):
	l1_inp = open(l1_input, "r")
	l2_inp = open(l2_input, "r")
	l1_out = open(l1_output, "w")
	l2_out = open(l2_output, "w")

	longest_l1 = 0
	longest_l2 = 0
	l1_lex_reps = []
	l2_lex_reps = []
	l1_lex_words = []
	l2_lex_words = []

	for _ in range (0, NUMBER_OF_WORDS): 
		l1_lex = l1_inp.readline().split("\n", 2)[0].lower()
		l2_lex = l2_inp.readline().split("\n", 2)[0].lower()

		l1_rep = []
		l2_rep = []
		for letter in l1_lex:
			l1_rep.append(ord(letter) - 95)
		for letter in l2_lex:
			l2_rep.append(ord(letter) - 95)

		l1_lex_words.append(l1_lex)
		l2_lex_words.append(l2_lex)
		l1_lex_reps.append(l1_rep)
		l2_lex_reps.append(l2_rep)

		if len(l1_lex) > longest_l1:
			longest_l1 = len(l1_lex)
		if len(l2_lex) > longest_l2:
			longest_l2 = len(l2_lex)
	print ("longest l1: {0} longest l2: {1}".format(longest_l1, longest_l2))
	
	for i in range(0, NUMBER_OF_WORDS):
		# add extra 0's here
		while len(l1_lex_reps[i]) < longest_l1:
			l1_lex_reps[i].append(0)
		l1_out.write("{0}\t{1}\n".format(l1_lex_words[i], " ".join([str(x) for x in l1_lex_reps[i]])))

		while len(l2_lex_reps[i]) < longest_l2:
			l2_lex_reps[i].append(0)
		l2_out.write("{0}\t{1}\n".format(l2_lex_words[i], " ".join([str(x) for x in l2_lex_reps[i]])))

	l1_inp.close()
	l2_inp.close()
	l1_out.close()
	l2_out.close()

if __name__ == "__main__":
	lex_reps("es_words.txt", "en_words.txt", "l1_lex.txt", "l2_lex.txt")

