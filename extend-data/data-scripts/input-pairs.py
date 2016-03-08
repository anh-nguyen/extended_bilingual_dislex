import sys

NUMBER_OF_WORDS = 60

def make_input_pairs (l1_lex_file, l2_lex_file, l1_phone_file, l2_phone_file, semantic_file, result_file):
	
	l1_lex_f = open(l1_lex_file, "r")
	l2_lex_f = open(l2_lex_file, "r")	
	l1_phone_f = open(l1_phone_file, "r")
	l2_phone_f = open(l2_phone_file, "r")
	sem = open(semantic_file, "r")
	result = open(result_file, "w")

	for _ in range (0, NUMBER_OF_WORDS): 
		l1_lex = l1_lex_f.readline().split("\n", 2)[0].lower()
		l2_lex = l2_lex_f.readline().split("\n", 2)[0].lower()
		l1_phone = l1_phone_f.readline().split(" ", 2)[0]
		l2_phone = l2_phone_f.readline().split(" ", 2)[0]
		sem_word = sem.readline().split(" ", 2)[0]
		result.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\n".format(l1_phone, l2_phone, l1_phone, l2_phone, l1_lex, l2_lex, sem_word))


	l1_lex_f.close()
	l2_lex_f.close()
	l1_phone_f.close()
	l2_phone_f.close()
	sem.close()
	result.close()


if __name__ == "__main__":
	make_input_pairs("es_words.txt", "en_words.txt", "l1.txt", "l2.txt", "semantic_reps.txt", "input_pairs.txt")














