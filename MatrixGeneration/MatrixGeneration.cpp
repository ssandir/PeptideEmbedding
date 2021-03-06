// MatrixGeneration.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include <math.h>
#include <algorithm>
#include <list>
#include <bitset>

#define const_n 1
#define const_k 4

using namespace std;

class Matrix {
public:
	vector<vector<int>> matrix;

	Matrix() {
		for (int i = 0; i < const_n * const_k * 2; ++i) {
			matrix.push_back(vector<int>(const_n * const_k * 2, 0));
		}
	}

	void add1Relation(int row, int column) {
		for (int i = 0; i < const_n*const_k * 2; ++i) {
			matrix[row][i]--;
		}
		for (int i = 0; i < const_n*const_k * 2; ++i) {
			matrix[i][column]--;
		}
		matrix[row][column] = 1;
	}

	void addRelation(int row, int column) {
		add1Relation(row, column);
		if (column % (const_n*const_k) != row % (const_n*const_k)) add1Relation(column, row); //simetry // (a,b) € R => (b,a) € R 
		//brez enakih členov antipararelno

		int othDirRow = (row + const_n * const_k) % (const_n*const_k * 2); // (a,b) € R => (-a,-b) € R
		int othDirColumn = (column + const_n * const_k) % (const_n*const_k * 2);
		add1Relation(othDirRow, othDirColumn);
		if (column % (const_n*const_k) != row % (const_n*const_k)) add1Relation(othDirColumn, othDirRow);
		//brez enakih členov antipararelno
	}

	void remove1Relation(int row, int column) {
		for (int i = 0; i < const_n*const_k * 2; ++i) {
			matrix[row][i]++;
		}
		for (int i = 0; i < const_n*const_k * 2; ++i) {
			matrix[i][column]++;
		}
		matrix[row][column] = 0;
	}

	void removeRelation(int row, int column) {
		remove1Relation(row, column);
		if (column % (const_n*const_k) != row % (const_n*const_k)) remove1Relation(column, row);

		int othDirRow = (row + const_n * const_k) % (const_n*const_k * 2); // (a,b) € R => (-a,-b) € R
		int othDirColumn = (column + const_n * const_k) % (const_n*const_k * 2);
		remove1Relation(othDirRow, othDirColumn);
		if (column % (const_n*const_k) != row % (const_n*const_k)) remove1Relation(othDirColumn, othDirRow);
	}

	bool hasRowRelation(int row) {
		for (int i = 0; i < const_n*const_k * 2; ++i) {
			if (matrix[row][i] == 1) return true;
		}
		return false;
	}

	Matrix* removeNegatives() {
		Matrix* nonNegative = new Matrix();
		for (int i = 0; i < const_n * const_k * 2; ++i) {
			for (int j = 0; j < const_n * const_k * 2; ++j) {
				nonNegative->matrix[i][j] = matrix[i][j] <= 0 ? 0 : 1;
			}
		}
		return nonNegative;
	}

	bool checkConnectivityCondition() {
		vector<vector<int>> connectGraph = vector<vector<int>>(const_n, vector<int>());
		vector<bool> checkVals = vector<bool>(const_n, false);

		for (int chain = 0; chain < const_n; ++chain) {
			for (int link = 0; link < const_k; ++link) {
				int pos = chain * const_k + link;
				for (int i = 0; i < const_n*const_k*2; ++i) {
					if (matrix[pos][i] == 1) {
						connectGraph[chain].push_back((i%(const_n*const_k)) / const_k); //chain se povezuje z verigo s členom i
						break;
					}
				}
			}
		}
		
		//naredimo obhod, graf mora biti povezan po verigah
		list<int> queue;
		queue.push_back(0);
		checkVals[0] = true;
		while (!queue.empty()) {
			int cChain = queue.front();
			queue.pop_front();
			for (int i = 0; i < connectGraph[cChain].size(); ++i) {
				if (!checkVals[connectGraph[cChain][i]]) {
					checkVals[connectGraph[cChain][i]] = true;
					queue.push_back(connectGraph[cChain][i]);
				}
			}
		}

		for (int i = 0; i < const_n; ++i) {
			if (!checkVals[i]) return false;
		}

		return true;
	}

	bool checkEquivalency(vector<Matrix*>* validMatrices, vector<int> perm, bitset<const_n> bitmask) {//returns true if equivalent matrix exists in validMatrices
		Matrix* eqMatrix = new Matrix();
		for (int i = 0; i < validMatrices->size(); ++i) {
			Matrix* compMatrix = (*validMatrices)[i];
			bool eq = true;
			for (int chain1 = 0; chain1 < const_n && eq; ++chain1) {
				for (int link1 = 0; link1 < const_k && eq; ++link1) {
					int posM1 = const_k * chain1 + link1;
					int posEqM1 = const_k * perm[chain1] + (bitmask[chain1] ? const_k - link1 - 1 + const_n*const_k: link1);
					for (int chain2 = 0; chain2 < const_n && eq; ++chain2) {
						for (int link2 = 0; link2 < const_k && eq; ++link2) {
							int posM2 = const_k * chain2 + link2;
							int posEqM2 = const_k * perm[chain2] + (bitmask[chain2] ? const_k - link2 - 1 + const_n * const_k: link2);
							if (matrix[posM1][posM2] != compMatrix->matrix[posEqM1][posEqM2] ||
							   matrix[posM1 + const_n * const_k][posM2] != compMatrix->matrix[(posEqM1 + const_n * const_k) % (2 * const_n*const_k)][posEqM2] ||
							   matrix[posM1 + const_n * const_k][posM2 + const_n * const_k] 
								!= compMatrix->matrix[(posEqM1 + const_n * const_k) % (2 * const_n*const_k)][(posEqM2 + const_n * const_k) % (2 * const_n*const_k)]) {
								eq = false;	
							}
							
						}
					}
				}
			}
			if (eq) {
				return true;
			}
		}
		return false;
	}

	void printMatrix(string location) {
		ofstream file;
		file.open(location);

		for (int chain = 0; chain < const_n; ++chain) {
			for (int link = 0; link < const_k; ++link) {
				file << "v" << chain + 1 << link + 1 << "  ";
				int pos = chain * const_k + link;

				for (int i = 0; i < const_n*const_k * 2; ++i) {
					file << matrix[pos][i] << " ";
				}
				file << endl;
			}
		}
		for (int chain = 0; chain < const_n; ++chain) {
			for (int link = 0; link < const_k; ++link) {
				file << "-v" << chain + 1 << link + 1 << " ";
				int pos = chain * const_k + link + const_n * const_k;

				for (int i = 0; i < const_n*const_k * 2; ++i) {
					file << matrix[pos][i] << " ";
				}
				file << endl;
			}
		}

		file.close();
	}
};



void saveMatrix(vector<Matrix*>* validMatrices, Matrix* cMatrix) {
	Matrix* nnMatrix = cMatrix->removeNegatives();//non negative matrix

	if (!nnMatrix->checkConnectivityCondition()) return;


	//PREVERI RAZLIČNE MOŽNOSTI EKVIVALENC PREDEN SHRANIŠ V MATRICES
	vector<int> perm;
	for (int i = 0; i < const_n; ++i) {
		perm.push_back(i);
	}

	do {//zamenjava verig
		for (int mask = 0; mask < (1 << const_n); ++mask) {//katero verigo obrnemo
			bitset<const_n> bitmask(mask);
			if (nnMatrix->checkEquivalency(validMatrices, perm, bitmask)) {
				return;
			}
		}
	} while (next_permutation(perm.begin(), perm.end()));

	validMatrices->push_back(nnMatrix);
}

void makeRelations(vector<Matrix*>* validMatrices, Matrix* cMatrix, int row) { //shrani vse matrike v validMatrices
	for (int i = 0; i < const_n*const_k * 2; ++i) {
		if (cMatrix->matrix[row][i] == 0 && i != row) { //še ni 1 v vrstici ali stolpcu, nimamo dveh enakih členov pararelno
			cMatrix->addRelation(row, i);

			int k = row+1;
			while (k < const_n * const_k * 2 && cMatrix->hasRowRelation(k)) {
				++k;
			}
			if (k == const_n * const_k * 2) {
				saveMatrix(validMatrices, cMatrix);
			}
			else makeRelations(validMatrices, cMatrix, k);

			cMatrix->removeRelation(row, i);
		}
	}
}

void printValidMatrices(vector<Matrix*>* validMatrices) {
	for (int i = 0; i < validMatrices->size(); ++i) {
		string location = ("matrices/i");
		location += to_string(i) + ".txt";
		(*validMatrices)[i]->printMatrix(location);
	}
}

int main()
{ 
	vector<Matrix*>* validMatrices = new vector<Matrix*>();
	Matrix* cMatrix = new Matrix();
	makeRelations(validMatrices, cMatrix, 0);

	printValidMatrices(validMatrices);


	cout << endl;
    return 0;
}

