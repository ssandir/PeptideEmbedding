// summarize.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include <math.h>

#define UNSTABLE 0
#define AMBIGOUS 1
#define IMPOSSIBLE 2

using namespace std;

int main()
{
	ofstream summary;
	ifstream results;
	results.open("results.txt");
	summary.open("summary.txt");
	vector<int> count(3, 0);
	vector<int> timesSum(3, 0);
	vector<int> timesMax(3, -1);
	vector<int> timesMin(3, 1000000000);
	string type;
	int t;
	while (results >> type >> type >> type >> t) {
		int tp = 0;
		if (type == "UNSTABLE") tp = 0;
		else if (type == "AMBIGOUS") tp = 1;
		else if (type == "IMPOSSIBLE") tp = 2;

		count[tp]++;
		timesSum[tp] += t;
		if (timesMax[tp] < t) timesMax[tp] = t;
		if (timesMin[tp] > t) timesMin[tp] = t;
	}

	summary << "type          n       avg      max         min" <<endl;
	summary << "UNSTABLE     " << count[0] << "    " << timesSum[0] / count[0] << "    " << timesMax[0] << "    " << timesMin[0] <<endl;
	summary << "AMBIGOUS     " << count[1] << "    " << timesSum[1] / count[1] << "    " << timesMax[1] << "    " << timesMin[1] <<endl;
	summary << "IMPOSSIBLE   " << count[2] << "    " << timesSum[2] / (count[2]+0.00001) << "    " << timesMax[2] << "    " << timesMin[2] <<endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
