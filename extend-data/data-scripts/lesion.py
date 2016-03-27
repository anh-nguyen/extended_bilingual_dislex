from __future__ import division
import sys
from random import gauss
import os.path

GAUSSIAN_MEAN = 0

def lesion_assocs (patient_file, lesion_file, map_sizes, num_reps, noise_amount, overwrite):
	
	if os.path.isfile(lesion_file) and not overwrite:
		print ("lesion_file {} already exists... not overwriting\n".format(lesion_file))
		return
	patient = open(patient_file, "r")
	lesioned = open(lesion_file, "w")

	line_num = 0

	for line in patient:				# read until we get to network weights
		line_num += 1
		lesioned.write(line)
		if "network-weights" in line:
			break

	l1_lines = (map_sizes["l1"]**2)*num_reps["l1lex"] + (map_sizes["l1"]**2)*num_reps["l1phonol"] + (map_sizes["l1"]**2)*num_reps["l1phonetic"]
	l2_lines = (map_sizes["l2"]**2)*num_reps["l2lex"] + (map_sizes["l2"]**2)*num_reps["l2phonol"] + (map_sizes["l2"]**2)*num_reps["l2phonetic"] 
	assocs_start =  line_num + l1_lines + l2_lines + (map_sizes["sem"]**2)*num_reps["sem"] + 1

	# order in the file: sl1 assocs, sl2 assocs, l1l2 assocs

	sl1lex_start = assocs_start
	sl2lex_start = sl1lex_start + (map_sizes["l1"]**2)*(map_sizes["sem"]**2)*2 
	l1l2lex_start = sl2lex_start + (map_sizes["l2"]**2)*(map_sizes["sem"]**2)*2 
	l1lexphonol_start = l1l2lex_start + (map_sizes["l1"]**2)*(map_sizes["l2"]**2)*2 
	l2lexphonol_start = l1lexphonol_start + (map_sizes["l1"]**2)*(map_sizes["l1"]**2)*2
	l2lexphonol_exclusive_end = l2lexphonol_start + + (map_sizes["l2"]**2)*(map_sizes["l2"]**2)*2

	# iterate until we reach assoc weights
	for line in patient:	# starting from network-weights
		line_num += 1
		lesioned.write(line)
		if line_num == sl1lex_start - 1:
			break

	# add noise to sl1 weights
	for line in patient:
		line_num += 1
		orig = float(line.strip())
		with_noise = orig + gauss(GAUSSIAN_MEAN, noise_amount["sl1lex"])
		with_noise = with_noise if with_noise > 0 else 0
		lesioned.write("{:.6f}\n".format(with_noise))
		if line_num == sl2lex_start - 1:
			break

	# add noise to sl2 weights
	for line in patient:
		line_num += 1
		orig = float(line.strip())
		with_noise = orig + gauss(GAUSSIAN_MEAN, noise_amount["sl2lex"])
		with_noise = with_noise if with_noise > 0 else 0
		lesioned.write("{:.6f}\n".format(with_noise))
		if line_num == l1l2lex_start - 1:
			break

	# iterate again until we reach the lex phonol weights
	for line in patient:
		line_num += 1
		lesioned.write(line)
		if line_num == l1lexphonol_start - 1:
			break

	# add noise to L1 lex phonol weights
	for line in patient:
		line_num += 1
		orig = float(line.strip())
		with_noise = orig + gauss(GAUSSIAN_MEAN, noise_amount["l1lexphonol"])
		with_noise = with_noise if with_noise > 0 else 0
		lesioned.write("{:.6f}\n".format(with_noise))
		if line_num == l2lexphonol_start - 1:
			break

	# add noise to L2 lex phonol weights
	for line in patient:
		line_num += 1
		orig = float(line.strip())
		with_noise = orig + gauss(GAUSSIAN_MEAN, noise_amount["l2lexphonol"])
		with_noise = with_noise if with_noise > 0 else 0
		lesioned.write("{:.6f}\n".format(with_noise))
		if line_num == l2lexphonol_exclusive_end - 1:
			break

	# continue writing the rest
	for line in patient:
		lesioned.write(line)


if __name__ == "__main__":
	map_sizes = {"l1": 20, "l2": 20, "sem": 20}
	num_reps = {"l1phonetic": 208, "l2phonetic": 120, "l1lex": 18, "l2lex": 11, "l1phonol": 17, "l2phonol": 12, "sem": 81}

	UTBA18_noise_amount = {'sl1lex': 0.32, 'sl2lex': 0.33, 'l1lexphonol' : 0, 'l2lexphonol' : 0}
	BUBA01_noise_amount = {'sl1lex': 0, 'sl2lex': 0, 'l1lexphonol' : 0, 'l2lexphonol' : 0}
	UTBA20_noise_amount = {'sl1lex': 0, 'sl2lex': 0, 'l1lexphonol' : 0, 'l2lexphonol' : 0}
	UTBA21_noise_amount = {'sl1lex': 0, 'sl2lex': 0, 'l1lexphonol' : 0, 'l2lexphonol' : 0}

	UTBA18_overwrite = True
	BUBA01_overwrite = False
	UTBA20_overwrite = False
	UTBA21_overwrite = False

	lesion_assocs("../../UTBA18-simu", "../../UTBA18-lesioned-simu", map_sizes, num_reps, UTBA18_noise_amount, UTBA18_overwrite)
	#lesion_assocs("../../BUBA01-simu", "../../BUBA01-lesioned-simu", map_sizes, num_reps, BUBA01_noise_amount, BUBA01_overwrite)
	#lesion_assocs("../../UTBA20-simu", "../../UTBA20-lesioned-simu", map_sizes, num_reps, UTBA20_noise_amount, UTBA20_overwrite)
	#lesion_assocs("../../UTBA21-simu", "../../UTBA21-lesioned-simu", map_sizes, num_reps, UTBA21_noise_amount, UTBA21_overwrite)









