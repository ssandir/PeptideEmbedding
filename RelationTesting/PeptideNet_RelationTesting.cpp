// PeptideNet_RelationTesting.cpp : Defines the entry point for the console application.
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
#include <time.h>

#define const_n 1
#define const_k 4
#define const_radius 3 //how far we go from position (0,0)
#define const_extend 1
#define conts_linkPairsPerJoint 4
#define const_minTermCount 3

#define EFFECTED_AREA 2

//vrednosti, ki jih vrne evaluateRelationMatrix:
#define UNSTABLE -2
#define AMBIGOUS -1
#define IMPOSSIBLE 0
#define SOLUTIONFOUND 1
#define RETURN_TO_RADIUS 2

//does link point towards center vertex?
#define LP_UNDEFINED -1
#define LP_OUT 0
#define LP_CENTER 1

using namespace std;

//debug print 
#define USEDEBUG
#ifdef USEDEBUG
#define DEBUGPRINT( ... )  printf(__VA_ARGS__)
#else
#define DEBUGPRINT( ... )
#endif




vector<pair<int, int>> allDirections = { make_pair(1,0), make_pair(0,1), make_pair(-1,0), make_pair(0,-1) };

int manhattanDistance(int x1, int y1, int x2, int y2) {
	return abs(x1 - x2) + abs(y1 - y2);
}

class NetVertex {
public:
	pair<int, int> linksUp;
	pair<int, int> linksRight;
	int jointSet;

	NetVertex(){
		linksUp = make_pair(-1,-1);
		linksRight = make_pair(-1, -1);
		jointSet = -1;
	}

	void embedLinkRight(int link) {
		if (linksRight.first == -1) {
			linksRight.first = link;
		}
		else linksRight.second = link;
	}

	void embedLinkUp(int link) {
		if (linksUp.first == -1) {
			linksUp.first = link;
		}
		else linksUp.second = link;
	}

	void removeLinkRight(int link) {
		if (linksRight.first == link) {
			linksRight.first = -1;
		}
		else if (linksRight.second == link) {
			linksRight.second = -1;
		}
		//else return false;
	}

	void removeLinkUp(int link) {
		if (linksUp.first == link) {
			linksUp.first = -1;
		}
		else if (linksUp.second == link) {
			linksUp.second = -1;
		}
		//else return false;
	}
};

class Net {
	vector<vector<NetVertex>> netStructure;
	int centerD = const_radius + const_k + const_extend;

public:
	int VertexVisitID;
	int LinksVisitID;

	Net() {
		netStructure = vector<vector<NetVertex>>((centerD + 1) * 2);
		for (int i = 0; i < (centerD + 1) * 2; i++) {
			netStructure[i] = vector<NetVertex>((centerD + 1) * 2);
			for (int j = 0; j < (centerD + 1) * 2; ++j) {
				netStructure[i][j] = NetVertex();
			}
		}
		VertexVisitID = 1;
		LinksVisitID = 1;
	}

	int findJointSet(int x, int y) {
		return netStructure[x + centerD][y + centerD].jointSet;
	}

	bool normalizePos(int* x1, int* y1, int* x2, int* y2) {
		if (abs(*x1 - *x2) + abs(*y1 - *y2) != 1) {
			DEBUGPRINT("Find link in net error: wrong coordinates \n");
			return false;
		}
		if (*x2 < *x1) {
			int tmp = *x1;
			*x1 = *x2;
			*x2 = tmp;
		}
		if (*y2 < *y1) {
			int tmp = *y1;
			*y1 = *y2;
			*y2 = tmp;
		}
		*x1 += centerD;
		*y1 += centerD;
		*x2 += centerD;
		*y2 += centerD;
		return true;
	}

	pair<int, int> findLinksOn(int x1, int y1, int x2, int y2) {
		if (normalizePos(&x1, &y1, &x2, &y2)) {
			return x2 > x1 ? netStructure[x1][y1].linksRight : netStructure[x1][y1].linksUp;
		}
		else return make_pair(-1, -1);
	}

	bool embedJointSet(int jointSet, int x, int y) {
		if (findJointSet(x, y) != -1) {
			DEBUGPRINT("embedJointSet error, spot already filled %d %d %d \n", jointSet, x, y);
			return false;
		}
		netStructure[x + centerD][y + centerD].jointSet = jointSet;
		return true;
	}

	bool embedLink(int linkID, int x1, int y1, int x2, int y2) {//naj vrne če že je kaj na tem mestuu?
		if (normalizePos(&x1, &y1, &x2, &y2)) {
			if (x2 > x1) {
				netStructure[x1][y1].embedLinkRight(linkID);
			}
			else {
				netStructure[x1][y1].embedLinkUp(linkID);
			}
			//warning for already embeded links
			return true;
		}
		else return false;

		
	}

	void removeLink(int x1, int y1, int x2, int y2, int linkID) {
		normalizePos(&x1, &y1, &x2, &y2);
		if (x2 > x1) {
			netStructure[x1][y1].removeLinkRight(linkID);
		}
		else {
			netStructure[x1][y1].removeLinkUp(linkID);
		}
 	}

	void removeJoint(int x, int y) {
		netStructure[x + centerD][y + centerD].jointSet = -1;
	}

	int findOneLink(pair<int,int> twoLinks) {
		return twoLinks.first == -1 ? twoLinks.second : twoLinks.first;
	}

	int findOneLink(int x1, int y1, int x2, int y2) {
		return findOneLink(findLinksOn(x1, y1, x2, y2));
	}
};


class Restriction {
public:
	int posX, posY, distance;

	Restriction(int x, int y,int d) {
		posX = x;
		posY = y;
		distance = d;
	}

	Restriction moveRestrictionUp() {
		return Restriction(posX, posY, distance + 1);
	}

	bool check(int x, int y) {
		return manhattanDistance(x, y, posX, posY) <= distance;
	}
};

class Link;
class Joint;
class JointSet;


class JointSet {
private:
	vector<Joint*> joints;

public:
	int id;
	int conLinksN;
	int posX, posY;
	bool embeded, inUse;
	int centerDistance;
	int terminateCount;
	vector<Restriction> restrictions;
	int visitID;
	int visitID2;
	
	int size() {
		return joints.size();
	}

	void add(Joint* joint);

	void remove(Joint* joint);

	Joint* jointAt(int i) {
		return joints[i];
	}

	void removeAt(int i);

	void merge(JointSet* jointSet);

	void separate(JointSet* unusedSet);

	int countSave, jointSave;
	pair<Link*, Joint*> conLinkJointAt(int pos);

	Link* conLinkAt(int pos) {
		return conLinkJointAt(pos).first;
	}

	void unembed() {
		embeded = false;
	}

	void embed(int x, int y) {
		posX = x;
		posY = y;
		embeded = true;
	}

	

	JointSet(int whatID) {
		id = whatID;
		embeded = false;
		visitID = 0;
		inUse = true;
		posX = -1000000;
		posY = -1000000;
		conLinksN = 0;
		centerDistance = 1000000000;
		countSave = 0;
		jointSave = 0;
		terminateCount = 0;
		visitID2 = 0;
	}
};

class Joint {
public:
	int id;
	vector <Link*> conLinks;
	bool visit;
	int visitID;
	JointSet* jointSet;

	Joint(int i){
		visit = false;
		id = i;
		visitID = 0;
	}

	int posX() {
		return jointSet->posX;
	}
	

	int posY() {
		return jointSet->posY;
	}

	bool embeded() {
		return jointSet->embeded;
	}
};

class Link {
public:
	int id;
	Joint * nJoint; //next
	Joint* pJoint;  //previous
	Link* boundPair;
	bool visit, embeded;
	int visitID;

	int chainType;
	int linkType;

	vector<pair<int, int>> directions;

	Link(int c, int l, int i) {
		boundPair = nullptr;
		visit = false;
		embeded = false;
		id = i;
		chainType = c;
		linkType = l;
		directions = { make_pair(1,0), make_pair(0,1), make_pair(-1,0), make_pair(0,-1) };  //right, up, left, down
		visitID = 0;
	}

	Link* nextLink() {
		if (nJoint->conLinks.size() == 1) return nullptr;
		if (nJoint->conLinks[0]->id == id) return nJoint->conLinks[1];
		return nJoint->conLinks[0];
	}

	Link* prevLink() {
		if (pJoint->conLinks.size() == 1) return nullptr;
		if (pJoint->conLinks[0]->id == id) return pJoint->conLinks[1];
		return pJoint->conLinks[0];
	}

	Joint* otherJoint(Joint* joint) {
		if (nJoint == joint) {
			return pJoint;
		}
		else if (pJoint == joint) {
			return nJoint;
		}
		else {
			return nullptr;
		}
	}

	JointSet* otherJointSet(JointSet* jointSet) {
		if (nJoint->jointSet == jointSet) {
			return pJoint->jointSet;
		}
		else if (pJoint->jointSet == jointSet) {
			return nJoint->jointSet;
		}
		else {
			return nullptr;
		}
	}

	int centerDistance() {
		return min(pJoint->jointSet->centerDistance, nJoint->jointSet->centerDistance);
	}
};

//jointSet functions
	inline void JointSet::add(Joint* joint) {
		joints.push_back(joint);
		conLinksN += joint->conLinks.size();
	}

	inline void JointSet::remove(Joint* joint) {
		joints.erase(std::remove(joints.begin(), joints.end(), joint), joints.end());
		conLinksN -= joint->conLinks.size();
	}

	inline void JointSet::removeAt(int i) {
		joints.erase(joints.begin() + i);
		conLinksN -= joints[i]->conLinks.size();
	}

	inline void JointSet::merge(JointSet* jointSet) { //always merge embeded <- nonimbeded, cant merge 2 embeded
		jointSet->inUse = false;
		for (int i = 0; i < jointSet->size(); ++i) {
			add(jointSet->jointAt(i));
			jointSet->jointAt(i)->jointSet = this;
		}
		terminateCount += jointSet->terminateCount;
	}

	inline void JointSet::separate(JointSet* unusedSet) { //mora biti vedno v obratnem zaporedju kot merge
		for (int i = unusedSet->size() - 1; i >= 0; --i) {
			if (joints.back() == unusedSet->jointAt(i)) { //če gremo v obratni smeri manjša čas zahtevnost
				joints.pop_back();
				conLinksN -= unusedSet->jointAt(i)->conLinks.size();
			}
			else remove(unusedSet->jointAt(i));
			unusedSet->jointAt(i)->jointSet = unusedSet;
		}
		unusedSet->inUse = true;
		terminateCount -= unusedSet->terminateCount;
		terminateCount = terminateCount > 0 ? terminateCount : 0;
	}

	inline pair<Link*, Joint*> JointSet::conLinkJointAt(int pos) {
		if (countSave > pos) {
			countSave = 0;
			jointSave = 0;
		}
		while (joints[jointSave]->conLinks.size() + countSave <= pos) {
			countSave += joints[jointSave]->conLinks.size();
			jointSave++;
		}
		Link* link = joints[jointSave]->conLinks[pos - countSave];
		Joint* joint = joints[jointSave];
		return make_pair(link, joint);
	}
//jointSet Functions

class RelMatrix {
public:
	bool** matrix;
	vector<int> relVector;

	RelMatrix() {
		matrix = new bool*[const_n*const_k * 2];
	}

	bool readRelMatrix(string fname) {
		ifstream inFile;
		inFile.open(fname);
		if (!inFile) {
			DEBUGPRINT("Can't open file %s \n",fname);
			return false;
		}
		string inStr;
		int i = 0, j = 0;
		matrix[i] = new bool[const_n*const_k * 2];
		while (inFile >> inStr && i < const_n * const_k * 2) { //presledek je separator, bere po besedah
			if (inStr == "0") {
				matrix[i][j] = false;
				DEBUGPRINT("0 ");
				j++;
			}
			if (inStr == "1") {
				matrix[i][j] = true;
				DEBUGPRINT("1 ");
				j++;
			}
			if (j == const_n * const_k * 2) {
				j = 0;
				i++;
				matrix[i] = new bool[const_n*const_k * 2];
				DEBUGPRINT("\n");
			}
		}
		inFile.close();
		if (i != const_n * const_k * 2 || j != 0) return false;
		return true;
	}

	bool transformRelMatrix() {
		for (int i = 0; i < 2 * const_n*const_k; ++i) {
			for (int j = 0; j < 2 * const_n*const_k; ++j) {
				if (matrix[i][j]) relVector.push_back(j);
			}
		}
		if (relVector.size() != 2 * const_n*const_k) {
			DEBUGPRINT("Incorrect matrix \n");
			return false;
		}
		return true;
	}

	int findCounterLink(Link* link) {
		int entry = link->chainType * const_k + link->linkType;
		return relVector[entry];
	}

	int counterChainType(Link* link) {
		int tableEntry = findCounterLink(link) % (const_n*const_k);
		return tableEntry / const_k;
	}

	int counterLinkType(Link* link) {
		return findCounterLink(link) % const_k;
	}

	int connectsParalel(Link* link) {
		int tableEntry = findCounterLink(link);
		return tableEntry < const_n*const_k;
	}
};

class Revert {
public:
	JointSet * targetJointSet;
	JointSet* unusedJointSet;
	int type;

	Revert(JointSet* mJointSet, JointSet* uJointSet) { //join
		targetJointSet = mJointSet;
		unusedJointSet = uJointSet;
		type = 2;
	}

	Revert(JointSet* embededJointSet) { //embed
		targetJointSet = embededJointSet;
		type = 1;
	}

	Revert() { //fix space
		type = 0;
	}
};

class Graph {
public:
	vector<Joint*> joints;
	vector<Link*> links;
	vector<JointSet*> jointSets;
	int jointVisitID;
	int linkVisitID;
	int jointSetVisitID;
	int jointSetVisitID2;

	Link* getLink(int id) {
		if (id == -1) return nullptr;
		return links[id];
	}

	Joint* getJoint(int id) {
		if (id == -1) return nullptr;
		return joints[id];
	}

	JointSet* getJointSet(int id) {
		if (id == -1) return nullptr;
		return jointSets[id];
	}

	Link* addChain(int chainType, int linkType) {
		joints.push_back(new Joint(joints.size()));//nov joint
		jointSets.push_back(new JointSet(jointSets.size())); //nov jointset
		
		for (int i = 0; i < const_k; ++i) {
			links.push_back(new Link(chainType, i, links.size())); //nov link
			links.back()->pJoint = joints.back(); //linku priredi previous joint
			joints.back()->conLinks.push_back(links.back()); //jointu priredi link
			jointSets.back()->add(joints.back());//joint setu dodaj joint (po dodajanju linka)
			joints.back()->jointSet = jointSets.back();//jointu priredi joint set
			joints.push_back(new Joint(joints.size())); //nov joint
			jointSets.push_back(new JointSet(jointSets.size())); //nov jointset
			links.back()->nJoint = joints.back(); //linku priredi naslednji joint
			joints.back()->conLinks.push_back(links.back()); //jointu dodaj link
		}
		jointSets.back()->add(joints.back()); //joint setu dodaj joint (po dodajanju linka)
		joints.back()->jointSet = jointSets.back();//jointu priredi joint set
		return getLink(links.size() - const_k + linkType);
	}

	void removeLastChain() {
		for (int i = 0; i < 5; ++i) {
			delete joints.back();
			joints.pop_back();
			delete jointSets.back();
			jointSets.pop_back();
			//if (links.back()->firstConnect != nullptr)links.back()->firstConnect->terminateCount += links.back()->terminateCount/2;
			if (i < 4) {
				delete links.back();
				links.pop_back();
			}
		}
		//zandji chain ma vedno 5 jointov, ki ima vsak joint set in 4 linke
	}

	Joint* jointByPosition(Link* link, int x, int y) {
		Joint* prevJoint = link->pJoint;
		Joint* nextJoint = link->nJoint;
		if (prevJoint->posX() == x && prevJoint->posY() == y) {
			return prevJoint;
		}
		else if (nextJoint->posX() == x && nextJoint->posY() == y) {
			return nextJoint;
		}
		DEBUGPRINT("Invalid jointByPosition query %d %d %d \n", link, x, y);
		return nullptr;
	}

	JointSet* jointSetByPosition(Link* link, int x, int y) {
		return jointByPosition(link, x, y)->jointSet;
	}

	bool checkLinkAttract_UnfixedJoints(Link* link1, JointSet* jointSet1, Link* link2, JointSet* jointSet2, RelMatrix* relMatrix) {
		if (jointSet1 == link1->pJoint->jointSet && jointSet2 == link2->pJoint->jointSet ||
			jointSet1 == link1->nJoint->jointSet && jointSet2 == link2->nJoint->jointSet) {
			return relMatrix->matrix[link1->chainType*const_k + link1->linkType][link2->chainType*const_k + link2->linkType];
		}
		else {
			return relMatrix->matrix[link1->chainType*const_k + link1->linkType][link2->chainType*const_k + link2->linkType + const_n * const_k];
		}
	}

	bool checkLinkAttract_FixedJoint(Link* link1, Link* link2, JointSet* jointSet, RelMatrix* relMatrix) { //linku 2 pripada joint, joint mora imeti določeno pozicijo, link1 mora imeti isti joint, ali joint na isti poziciji 
		JointSet* jointOnSamePos = jointSetByPosition(link1, jointSet->posX, jointSet->posY);
		return checkLinkAttract_UnfixedJoints(link1, jointOnSamePos, link2, jointSet, relMatrix);
	}

	bool areJointsOnDistance(JointSet* jointSet, JointSet* searchedJointSet, int distance) {//<-here remove distance, function into graph
		for (int i = 0; i < jointSet->conLinksN; ++i) {
			JointSet* nJointSet = jointSet->conLinkAt(i)->otherJointSet(jointSet);
			if (nJointSet->visitID2 < jointSetVisitID2) {
				if (nJointSet == searchedJointSet && (distance == 1 || distance == 3)) return false;
				if (distance > 1) {
					nJointSet->visitID2 = jointSetVisitID2;
					if (!areJointsOnDistance(nJointSet, searchedJointSet, distance - 1)) return false;
				}
			}
		}
		return true;
	}

	bool checkIfMatch(list<pair<int, bool>>* unboundLinks, int linkEntry, bool orientToJS, RelMatrix* relMatrix) {
		for (list<pair<int, bool>>::iterator it = unboundLinks->begin(); it != unboundLinks->end(); ++it) {
			if (orientToJS == it->second && relMatrix->matrix[it->first][linkEntry] ||
				orientToJS != it->second && relMatrix->matrix[it->first][linkEntry + const_n * const_k]) {
				unboundLinks->erase(it);
				return true;
			}
		}
		return false;
	}

	bool checkIfMatchNoDir(list<int>* unboundLinks, int linkEntry, RelMatrix* relMatrix) {
		for (list<int>::iterator it = unboundLinks->begin(); it != unboundLinks->end(); ++it) {
			if (relMatrix->matrix[*it][linkEntry] || relMatrix->matrix[*it][linkEntry + const_n * const_k]) {
				unboundLinks->erase(it);
				return true;
			}
		}
		return false;
	}

	bool checkLinkPairNumberInJoints(JointSet* jointSet, RelMatrix* relMatrix) {
		linkVisitID++;
		int pairCount = 0;
		list<pair<int, bool>> unboundLinks = list<pair<int, bool>>(); //link table entry, bool := is link oriented towards jointSet?
		for (int i = 0; i < jointSet->conLinksN || unboundLinks.size()>0; ++i) {
			if (i < jointSet->conLinksN) {
				Link* link = jointSet->conLinkAt(i);
				if (link->visitID < linkVisitID) {
					link->visitID = linkVisitID;
					if (link->boundPair != nullptr) {
						pairCount++;
						link->boundPair->visitID = linkVisitID;
					}
					else {
						bool orientToJS = link->nJoint->jointSet == jointSet;
						int linkEntry = link->chainType*const_k + link->linkType;
						if (!checkIfMatch(&unboundLinks, linkEntry, orientToJS, relMatrix)) {
							pairCount++;
							unboundLinks.push_back(make_pair(linkEntry, orientToJS));
						}
					}
				}
			}
			else {
				int entry = unboundLinks.front().first;
				bool orientToJS = unboundLinks.front().second;
				unboundLinks.pop_front();

				int cEntry = relMatrix->relVector[entry];
				bool paralel = cEntry < const_n * const_k;

				int cEntryNext = -1;
				bool nextOrientToJS;
				if (paralel == orientToJS && cEntry%const_k < const_k - 1) {
					cEntryNext = (cEntry % (const_n*const_k)) + 1;
					nextOrientToJS = false;
				}
				else if (paralel != orientToJS && cEntry%const_k > 0) {
					cEntryNext = (cEntry % (const_n*const_k)) - 1;
					nextOrientToJS = true;
				}

				if (cEntryNext != -1) {
					if (!checkIfMatch(&unboundLinks, cEntryNext, nextOrientToJS, relMatrix)) {
						pairCount++;
						unboundLinks.push_back(make_pair(cEntryNext, nextOrientToJS));
					}
				}
			}
			if (pairCount > conts_linkPairsPerJoint) {
				return false;
			}
		}
		return true;
	}

	int countLinkNumberInJointSet(JointSet* jointSet) {
		linkVisitID++;
		int linkCount = 0;
		for (int i = 0; i < jointSet->conLinksN; ++i) {
			Link* link = jointSet->conLinkAt(i);
			if (link->visitID < linkVisitID) {
				link->visitID = linkVisitID;
				linkCount++;
				if (link->boundPair != nullptr) {
					link->boundPair->visitID = linkVisitID;
				}
			}
		}
		return linkCount;
	}

	list<Link*>* findHalfFixedLinks() {
		list<Link*>* halfFixedLinks = new list<Link*>();
		for (int i = 0; i < links.size(); ++i) {
			if (links[i]->nJoint->embeded() != links[i]->pJoint->embeded()) {
				halfFixedLinks->push_back(links[i]);
			}
		}
		return halfFixedLinks;
	}

	list<Link*>* findEffectedHalfFixedLinks_connect(Link* link) {
		list<Link*>* effectedLinks = new list<Link*>();

		jointSetVisitID++;
		linkVisitID++;
		list<JointSet*> jointSetQueue;
		jointSetQueue.push_back(link->pJoint->jointSet);
		jointSetQueue.push_back(link->nJoint->jointSet);
		link->pJoint->jointSet->visitID = jointSetVisitID;
		link->nJoint->jointSet->visitID = jointSetVisitID;

		while (!jointSetQueue.empty()) {
			JointSet* cJointSet = jointSetQueue.front();
			jointSetQueue.pop_front();

			for (int j = 0; j < cJointSet->conLinksN; ++j) {  //za vsak link v tem jointu
				Link* cLink = cJointSet->conLinkAt(j);

				if (cLink->visitID < linkVisitID) {
					cLink->visitID = linkVisitID;
					if (cLink->boundPair != nullptr) cLink->boundPair->visitID = linkVisitID;
					if (cLink->pJoint->embeded() != cLink->nJoint->embeded()) effectedLinks->push_back(cLink);
				}

				JointSet* nJointSet = link->otherJointSet(cJointSet);
				if (nJointSet->visitID < jointSetVisitID) {
					nJointSet->visitID = jointSetVisitID;
					if (!nJointSet->embeded) {
						jointSetQueue.push_back(nJointSet);
					}
				}

			}
		}
		return effectedLinks;
	}

	void propagateRestriction(list<JointSet*>* jointSetQueue) {
		while (!jointSetQueue->empty()) {
			JointSet* jointSet = jointSetQueue->front();
			jointSetQueue->pop_front();
			for (int j = 0; j < jointSet->conLinksN; ++j) {
				JointSet* nJointSet = jointSet->conLinkAt(j)->otherJointSet(jointSet);
				if (!nJointSet->embeded) {
					if (nJointSet->visitID < jointSetVisitID) {
						nJointSet->restrictions.push_back(jointSet->restrictions.back().moveRestrictionUp());
						nJointSet->visitID = jointSetVisitID;
						jointSetQueue->push_back(nJointSet);
					}
				}
			}
		}
	}

	void handleNewRestriction(JointSet* jointSet) {
		if (jointSet->embeded && jointSet->inUse) {
			jointSetVisitID++;
			list<JointSet*> jointSetQueue;
			jointSetQueue.push_back(jointSet);
			jointSet->restrictions.push_back(Restriction(jointSet->posX, jointSet->posY, 0));
			propagateRestriction(&jointSetQueue);
		}
	}

	void findRestrictions() {
		//clear all restrictions 1st
		for (int i = 0; i < jointSets.size(); ++i) {
			jointSets[i]->restrictions.clear();
		}
		//for  each fixed jointSet propagate its restriction to others
		for (int i = 0; i < jointSets.size(); ++i) {
			handleNewRestriction(jointSets[i]);
		}
	}

	bool checkCon2Stability(JointSet* jointSet) {//drugi pogoj stabilnosti: preverjamo povezanost G_t
		jointSetVisitID++;
		list<Link*> linkQueue;
		linkQueue.push_back(jointSet->conLinkAt(0));
		int jointCount = 1;
		linkQueue.front()->otherJointSet(jointSet)->visitID = jointSetVisitID;
		while (!linkQueue.empty()) {
			Link* link = linkQueue.front();
			linkQueue.pop_front();
			if (!link->embeded) return false;
			for (int i = 0; i < 2; ++i) { //za link in link->boundPair
				JointSet* nextJointSet = nullptr;
				Link* nextLink = link;
				if (link->nJoint->jointSet == jointSet && link->id%const_k < const_k - 1) {//naslednji link je v jointset
					nextJointSet = links[link->id + 1]->otherJointSet(jointSet);
					nextLink = links[link->id + 1];
				}
				else if (link->pJoint->jointSet == jointSet && link->id%const_k > 0) {//prejšnji link je v jointset
					nextJointSet = links[link->id - 1]->otherJointSet(jointSet);
					nextLink = links[link->id - 1];
				}
				if (nextJointSet != nullptr && nextJointSet->visitID < jointSetVisitID) {
					nextJointSet->visitID = jointSetVisitID;
					jointCount++;
					linkQueue.push_back(nextLink);
				}
				link = link->boundPair;
			}
		}
		return jointCount == 4;
	}

	Graph() {
		jointVisitID = 1;
		linkVisitID = 1;
		jointSetVisitID = 1;	
		jointSetVisitID2 = 1;
	}

	void printGraph() {
		DEBUGPRINT("\n CURRENT GRAPH \n \n JointSets: \n");
		for (int i = 0; i < jointSets.size(); ++i) {
			if (jointSets[i]->inUse) {
				DEBUGPRINT("JointSetID: %d, |centerDistance: %d |, conLinks:", jointSets[i]->id, jointSets[i]->centerDistance);
				for (int j = 0; j < jointSets[i]->conLinksN; ++j) {
					DEBUGPRINT("%d, ", jointSets[i]->conLinkAt(j)->id);
				}
				if (jointSets[i]->embeded) {
					DEBUGPRINT(" x: %d y: %d, ", jointSets[i]->posX, jointSets[i]->posY);
				}
				DEBUGPRINT("\n");
			}
		}
		DEBUGPRINT("\n Links: \n" );
		for (int i = 0; i < links.size(); i++) {
			DEBUGPRINT("LinkID: %d, pJointSet:%d, nJointSet:%d", links[i]->id, links[i]->pJoint->jointSet->id, links[i]->nJoint->jointSet->id);
			DEBUGPRINT(", boundPair:%d", links[i]->boundPair == nullptr ? -1 : links[i]->boundPair->id);
			DEBUGPRINT(", chainType:%d, linkType:%d, embeded:%d", links[i]->chainType, links[i]->linkType, links[i]->embeded);
			DEBUGPRINT("\n");
		}
		DEBUGPRINT("\n");
	}

	void printGraph(ofstream* proofF) {
		*proofF << "\n GRAPH \n \n JointSets: \n";
		for (int i = 0; i < jointSets.size(); ++i) {
			if (jointSets[i]->inUse) {
				*proofF << "JointSetID:  " << jointSets[i]->id << ", |centerDistance: "<< jointSets[i]->centerDistance <<" |, conLinks:";
				for (int j = 0; j < jointSets[i]->conLinksN; ++j) {
					*proofF <<  jointSets[i]->conLinkAt(j)->id<<", " ;
				}
				if (jointSets[i]->embeded) {
					*proofF <<  " x: "<< jointSets[i]->posX <<" y: "<< jointSets[i]->posY <<", ";
				}
				*proofF << "\n";
			}
		}
		DEBUGPRINT("\n Links: \n");
		for (int i = 0; i < links.size(); i++) {
			*proofF <<  "LinkID: "<< links[i]->id <<", pJointSet:"<< links[i]->pJoint->jointSet->id <<", nJointSet:"<< links[i]->nJoint->jointSet->id;
			*proofF << ", boundPair:" << (links[i]->boundPair == nullptr ? -1 : links[i]->boundPair->id);
			*proofF << ", chainType:"<< links[i]->chainType <<", linkType:"<< links[i]->linkType <<", embeded:"<< links[i]->embeded;
			*proofF << "\n";
		}
		*proofF << "\n";
	}
};

class ChainStructure {
public:
	Net net;
	Graph graph;

	bool initialized;
	bool fixedSpace;

	vector<list<Revert>> revertStack;

	bool checkNetPos(JointSet* fixedJointSet, JointSet*  freeJointSet, int cposX, int cposY, Link* link, RelMatrix* relMatrix) {
		pair<int, int> linksInNet = net.findLinksOn(fixedJointSet->posX, fixedJointSet->posY, cposX, cposY);

		if (linksInNet.first == link->id || linksInNet.second == link->id) return true;

		if (linksInNet.first == -1 && linksInNet.second == -1) return true;

		if (link->boundPair == nullptr) {
			int linkInNet = net.findOneLink(linksInNet);
			return graph.checkLinkAttract_FixedJoint(graph.links[linkInNet], link, fixedJointSet, relMatrix);
		}
		else if (link->boundPair->id == net.findOneLink(linksInNet)) return true;

		return false;
	}

	void unembedLink(Link* link) {
		if (link->embeded) {
			net.removeLink(link->nJoint->posX(), link->nJoint->posY(), link->pJoint->posX(), link->pJoint->posY(), link->id);
			link->embeded = false;
		}
	}

	void unembed(JointSet* jointSet) {
		jointSet->unembed();
		net.removeJoint(jointSet->posX, jointSet->posY);
		for (int i = 0; i < jointSet->conLinksN; ++i) {
			unembedLink(jointSet->conLinkAt(i));
		}
	}

	bool embedLink(Link* link, RelMatrix* relMatrix) {//both joint sets embeded
		JointSet* pJointSet = link->pJoint->jointSet;
		JointSet* nJointSet = link->nJoint->jointSet;
		if (!pJointSet->embeded || !nJointSet->embeded) return false;
		if (abs(pJointSet->posX - nJointSet->posX) + abs(pJointSet->posY - nJointSet->posY) != 1) return false;
		if (!checkNetPos(nJointSet, pJointSet, pJointSet->posX, pJointSet->posY, link, relMatrix))  return false;

		link->embeded = true;
		net.embedLink(link->id, nJointSet->posX, nJointSet->posY, pJointSet->posX, pJointSet->posY);//naj vrne, če že tam kaj je

		return true;
	}

	bool embed(JointSet* jointSet, int x, int y, RelMatrix* relMatrix) {
		jointSet->embed(x, y);

		net.embedJointSet(jointSet->id, x, y);

		for (int i = 0; i < jointSet->conLinksN; ++i) {
			Link* link = jointSet->conLinkAt(i);
			if (link->embeded) continue;
			JointSet* otherJointSet = link->otherJointSet(jointSet);
			if (otherJointSet->embeded) {
				if (!embedLink(link, relMatrix)) {
					unembed(jointSet);
					return false;
				}
			}
		}
		return true;
	}
	//ZDRUŽI 2 JOINTA, ki sta na istem mestu

	bool hasConCandidate(Link* link, RelMatrix* relMatrix) {
		int chainType = relMatrix->counterChainType(link);
		int linkType = relMatrix->counterLinkType(link);
		bool paralel = relMatrix->connectsParalel(link);

		for (int i = linkType; i < graph.links.size(); i += const_k) {
			Link* candidate = graph.getLink(i);
			if (chainType == candidate->chainType && link->id != i && candidate->boundPair == nullptr) { //osnovno ujemanje in prostost
				if (checkDistance(link, candidate, paralel) && checkEmbedMatch(link, candidate, paralel, relMatrix)) {
					//1 - preverjanje razdalje (med jointoma ne sme biti 1 ali 3
					//2 - preverjanje omejitev in vloženosti (postavimo nevložen joint na pozicijo vloženega)
					return true;
				}
			}
		}
		return false;
	}
	
	bool checkLinkPairNumberInArea(int cJS, int maxDistance, RelMatrix* relMatrix) {
		graph.jointSetVisitID++;
		graph.linkVisitID++;

		JointSet* cJointSet = graph.jointSets[cJS];
		cJointSet->centerDistance = 0;
		cJointSet->visitID = graph.jointSetVisitID;
		list<JointSet*> jointSetQueue;
		jointSetQueue.push_back(cJointSet);

		int linkNumOnDistance = 0;
		int cDistance = 0;
		list<int> unboundLinks = list<int>(); //linkID, oriented to center bool
		bool chk = true;

		while (!jointSetQueue.empty()) {
			cJointSet = jointSetQueue.front();
			jointSetQueue.pop_front();
			if (cJointSet->centerDistance > cDistance) {
				cDistance = cJointSet->centerDistance;
				unboundLinks.clear();
				linkNumOnDistance = 0;
			}

			for (int j = 0; j < cJointSet->conLinksN; ++j) {  //za vsak link v tem jointu
				Link* link = cJointSet->conLinkAt(j);

				if (link->visitID < graph.linkVisitID) {
					link->visitID = graph.linkVisitID;
					if (link->boundPair != nullptr) {
						link->boundPair->visitID = graph.linkVisitID;
						linkNumOnDistance++;
					}
					//else if (!hasConCandidate(link, relMatrix)) linkNumOnDistance++;
					else {
						int cEntry = link->chainType*const_k + link->linkType;
						if (!graph.checkIfMatchNoDir(&unboundLinks, cEntry, relMatrix)) {
							linkNumOnDistance++;
							unboundLinks.push_back(cEntry);
						}
					}
				}

				if (linkNumOnDistance > 4 + cDistance * 8) {
					return false;
				}

				JointSet* nJointSet = link->otherJointSet(cJointSet);
				if (nJointSet->visitID < graph.jointSetVisitID) {
					nJointSet->visitID = graph.jointSetVisitID;
					nJointSet->centerDistance = cJointSet->centerDistance + 1;
					if (nJointSet->centerDistance <= maxDistance) {
						jointSetQueue.push_back(nJointSet);
					}
				}
			}

		}
		return true;
	}

	Link* addCounterChain(RelMatrix* relMatrix, Link* link) {
		return graph.addChain(relMatrix->counterChainType(link), relMatrix->counterLinkType(link));
	}

	vector<Link*>* conCandidates(Link* link, RelMatrix* relMatrix) {
		int chainType = relMatrix->counterChainType(link);
		int linkType = relMatrix->counterLinkType(link);
		bool paralel = relMatrix->connectsParalel(link);
		vector<Link*>* candidates = new vector<Link*>();

		for (int i = linkType; i < graph.links.size(); i += const_k) {
			Link* candidate = graph.getLink(i);
			if (chainType == candidate->chainType && link->id != i && candidate->boundPair == nullptr) { //osnovno ujemanje in prostost
				if (checkDistance(link, candidate, paralel) && checkEmbedMatch(link, candidate, paralel, relMatrix)) {
					//1 - preverjanje razdalje (med jointoma ne sme biti 1 ali 3
					//2 - preverjanje omejitev in vloženosti (postavimo nevložen joint na pozicijo vloženega)
					candidates->push_back(candidate);
				}
			}
		}
		return candidates;
	}

	/*
	void calcCenterDistance(int linkID) {
	if (links[linkID].boundPair != -1) {
	int linkID2 = links[links[linkID].boundPair].id;
	int minD = min(links[linkID].centerDistance, links[linkID2].centerDistance);
	links[linkID].centerDistance=minD;
	links[linkID2].centerDistance = minD;
	}
	for (int i = 0; i < 2;++i) {       //za oba jointa linka
	int jointID = i == 0 ? links[linkID].pJoint : links[linkID].nJoint;
	for (int j = 0; j < joints[jointID].conLinks.size();++j) {  //za vsak link v tem jointu
	int nlinkID = joints[jointID].conLinks[j];
	if (links[nlinkID].centerDistance > links[linkID].centerDistance + 1) { //ki ima prevelik podatek o oddaljenosti (ne gremo nazaj)
	links[nlinkID].centerDistance = links[linkID].centerDistance + 1;
	calcCenterDistance(nlinkID);
	}
	}
	}
	}*/
	Link* findLinkToConnect(RelMatrix* relMatrix) {
		Link* rLink = nullptr;
		JointSet* maxTermCount = nullptr;

		JointSet* cJointSet = graph.getJointSet(net.findJointSet(0, 0));
		graph.jointSetVisitID++;
		cJointSet->centerDistance = 0;
		cJointSet->visitID = graph.jointSetVisitID;
		list<JointSet*> jointSetQueue;
		jointSetQueue.push_back(cJointSet);

		while (!jointSetQueue.empty()) {
			cJointSet = jointSetQueue.front();
			jointSetQueue.pop_front();

			for (int j = 0; j < cJointSet->conLinksN; ++j) {  //za vsak link v tem jointu
				Link* cLink = cJointSet->conLinkAt(j);


				if (cLink->boundPair == nullptr) {
					if (rLink == nullptr) {
						rLink = cLink;
						maxTermCount = cJointSet;
					}
					else if (maxTermCount->centerDistance < cJointSet->centerDistance) {
						return rLink;
					}
					else if (cJointSet->terminateCount >= const_minTermCount && maxTermCount->terminateCount <= cJointSet->terminateCount) {
						if (maxTermCount->terminateCount == cJointSet->terminateCount && rLink->otherJointSet(maxTermCount)->terminateCount >= cLink->otherJointSet(cJointSet)->terminateCount) continue;
						rLink = cLink;
						maxTermCount = cJointSet;
					}
				}

				JointSet* nJointSet = cLink->otherJointSet(cJointSet);
				if (nJointSet->visitID < graph.jointSetVisitID) {
					nJointSet->visitID = graph.jointSetVisitID;
					nJointSet->centerDistance = cJointSet->centerDistance + 1;
					if (nJointSet->centerDistance < const_radius + const_extend) {
						jointSetQueue.push_back(nJointSet);
					}
				}

			}
		}
		if (rLink != nullptr &&  maxTermCount->terminateCount >= 0) maxTermCount->terminateCount--;
		return rLink;
	}

	bool checkDistance(Link* link1, Link* link2, bool paralel) { 
		JointSet* l1pJoint = link1->pJoint->jointSet;
		JointSet* l1nJoint = link1->nJoint->jointSet;
		JointSet* l1pJointPair = (paralel ? link2->pJoint : link2->nJoint)->jointSet;
		JointSet* l1nJointPair = (paralel ? link2->nJoint : link2->pJoint)->jointSet;

		graph.jointSetVisitID2++;
		l1pJoint->visitID2 = graph.jointSetVisitID2;
		l1nJoint->visitID2 = graph.jointSetVisitID2;
		bool tmp = graph.areJointsOnDistance(l1pJoint, l1pJointPair, 3);
		graph.jointSetVisitID2++;
		l1pJoint->visitID2 = graph.jointSetVisitID2;
		l1nJoint->visitID2 = graph.jointSetVisitID2;
		return tmp && graph.areJointsOnDistance(l1nJoint, l1nJointPair, 3);
	}

	bool checkRestrictions(int x, int y, JointSet* jointSet, RelMatrix* relMatrix) {
		for (int i = 0; i < jointSet->restrictions.size();++i) {
			Restriction* restriction = &jointSet->restrictions[i];
			if (manhattanDistance(x, y, restriction->posX, restriction->posY) > restriction->distance) return false;
		}
		for (int i = 0; i < jointSet->conLinksN; ++i) {
			Link* link = jointSet->conLinkAt(i);
			JointSet* otherJointSet = link->otherJointSet(jointSet);
			if (otherJointSet->embeded) {
				if (abs(x - otherJointSet->posX) + abs(y - otherJointSet->posY) != 1) return false;
				pair<int,int> linksOnLoc = net.findLinksOn(x, y, otherJointSet->posX, otherJointSet->posY);
				if (linksOnLoc.first == -1 && linksOnLoc.second == -1) continue;
				if (linksOnLoc.first == link->id || linksOnLoc.second == link->id) continue;
				if (linksOnLoc.first != -1 && linksOnLoc.second != -1) return false;
				Link* linkOnLoc = graph.getLink(net.findOneLink(linksOnLoc));
				if (!graph.checkLinkAttract_FixedJoint(link, linkOnLoc, otherJointSet, relMatrix)) return false;
			}
		}
		return true;
	}

	bool checkEmbedMatch(Link* link1, Link* link2, bool paralel, RelMatrix* relMatrix) {
		Joint* l1pJoint = link1->pJoint;
		Joint* l1nJoint = link1->nJoint;
		Joint* l1pJointPair = (paralel ? link2->pJoint : link2->nJoint);
		Joint* l1nJointPair = (paralel ? link2->nJoint : link2->pJoint);

		if (l1pJoint->embeded() && l1pJointPair->embeded() && l1pJoint->jointSet != l1pJointPair->jointSet) return false; 
		if (l1nJoint->embeded() && l1nJointPair->embeded() && l1nJoint->jointSet != l1nJointPair->jointSet) return false;

		Joint* joint1 = l1pJoint->embeded() ? l1pJoint : l1pJointPair;
		Joint* joint2 = l1nJoint->embeded() ? l1nJoint : l1nJointPair;
		if (joint1->embeded() && joint2->embeded()) {
			if (abs(joint1->posX() - joint2->posX()) + abs(joint1->posY() - joint2->posY()) != 1) return false;
			pair<int, int> linksInNet = net.findLinksOn(joint1->posX(), joint1->posY(), joint2->posX(), joint2->posY());
			Link* linkInNet = graph.getLink(net.findOneLink(linksInNet));
			if (linkInNet != nullptr && linkInNet != link1 && linkInNet != link2) return false;
		}
		//poskrbimo za to, da nimamo dveh različnih vloženih jointov na isti lokaciji.  Če sta oba vložena in nista isti joint, pomeni, da sta na različni lokaciji.

		if (joint1->embeded()) {
			if (!checkRestrictions(joint1->posX(), joint1->posY(), joint1 == l1pJoint ? l1pJointPair->jointSet : l1pJoint->jointSet, relMatrix)) return false;
		}
		if (joint2->embeded()) {
			if (!checkRestrictions(joint2->posX(), joint2->posY(), joint2 == l1nJoint ? l1nJointPair->jointSet : l1nJoint->jointSet, relMatrix)) return false;
		}
		//preverjanje omejitev
		
		return true;
	}

	bool bindLinkPairs(JointSet* jointSet, RelMatrix* relMatrix) {// (link1 je boundPair link2 <=> linka imata enaka jointseta )
		//=>linka lahko postaneta bound pair le ko se spreminjajo jointSeti

		for (int i = 0; i < jointSet->conLinksN; ++i) {
			Link* link1 = jointSet->conLinkAt(i);
			for (int j = 0; j < jointSet->conLinksN; ++j) {
				if (j == i) continue;
				Link* link2 = jointSet->conLinkAt(j);
				if (link1->otherJointSet(jointSet) == link2->otherJointSet(jointSet)) {
					if (!graph.checkLinkAttract_UnfixedJoints(link1, jointSet, link2, jointSet, relMatrix))
						return false;
					if (link1->boundPair != link2 && link1->boundPair != nullptr) 
						return false;
					if (link2->boundPair != link1 && link2->boundPair != nullptr) 
						return false;
					link1->boundPair = link2;
					link2->boundPair = link1;
				}
			}
		}
		return true;
	}

	bool unbindLinkPairs(JointSet* jointSet) {// (link1 je boundPair link2 <=> linka imata enaka jointseta )
		//=>linka lahko postaneta bound pair le ko se spreminjajo jointSeti (merge, divide)
		for (int i = 0; i < jointSet->conLinksN; ++i) {
			Link* link = jointSet->conLinkAt(i);
			if (link->boundPair != nullptr) {
				if (link->boundPair->otherJointSet(jointSet) == nullptr || link->boundPair->otherJointSet(jointSet) != link->otherJointSet(jointSet)) {
					//link ima jointSet, če boundPair nima jointSet ali pa imata oba jointSet pa drug set ni enak, ju razvežemo
					link->boundPair->boundPair = nullptr;
					link->boundPair = nullptr;
				}
			}
		}
		return true;
	}

	bool joinJointSets(JointSet* jointSet1, JointSet* jointSet2, RelMatrix* relMatrix) {
		if (jointSet1 == jointSet2) return true;

		//if joint1.jeVložen in joint2.jeVložen in lokacija je ista <- do tega ne sme priti, to detektiramo pri vlaganju (2 jointa na istem mestu = združimo)
		if (jointSet2->embeded) {
			JointSet* tmp = jointSet2;
			jointSet2 = jointSet1;
			jointSet1 = tmp;
		}

		jointSet1->merge(jointSet2);
		


		if (!bindLinkPairs(jointSet1, relMatrix) //poveže linke z istimi jointi, max 2 privlačna or return false
			|| !graph.checkLinkPairNumberInJoints(jointSet1, relMatrix) //prešteje število (možnih) parov linkov v jointih
			) {
			jointSet1->separate(jointSet2);
			unbindLinkPairs(jointSet1);
			unbindLinkPairs(jointSet2);
			return false;
		}
		

		if (jointSet1->embeded) {
			for (int i = 0; i < jointSet2->conLinksN; ++i) {
				Link* link = jointSet2->conLinkAt(i);
				if (link->otherJointSet(jointSet1)->embeded && !link->embeded) { //če se zgodi da ima sedaj nevložen link 2 vložena jointa, ga vložimo
					embedLink(link, relMatrix); //vse pogoje v embed link smo že prej preverili
				}
			}
		}

		return true;
	}
	
	void separateJointSets(JointSet* jointSet1, JointSet* jointSet2) {//JS1=JS1-JS2
		if (jointSet1 == jointSet2) return;

		if (jointSet1->embeded) { //=> jointSet2 je !embeded
			for (int i = 0; i < jointSet2->conLinksN; ++i) {
				Link* link = jointSet2->conLinkAt(i);
				if (link->embeded) { //če se zgodi da ima sedaj vložen link nevložen joint, ga izložimo
					unembedLink(link);
				}
			}
		}

		jointSet1->separate(jointSet2);

		unbindLinkPairs(jointSet2);
		unbindLinkPairs(jointSet1);
		
		
	}

	bool revertConnectAndEmbed(RelMatrix* relMatrix) {//reverts all changes made in iteretion on current depth, returns false
		while (!revertStack.back().empty()) {
			Revert change = revertStack.back().back();
			revertStack.back().pop_back();
			switch (change.type) {
			case 0: 
				fixedSpace = false;
				break;
			case 1: 
				unembed(change.targetJointSet);
				break;
			case 2: 
				separateJointSets(change.targetJointSet, change.unusedJointSet);
				break;
			}
		}

		for (int i = 0; i < graph.links.size();++i) {
			graph.links[i]->directions = allDirections;
		}

		revertStack.pop_back();

		return false;
	}

	bool tryEmbedLinkDir(Link* link, JointSet* curJointSet, pair<int, int> dir, RelMatrix* relMatrix) {//link mora imeti 1 joint že embeded != curjount
		JointSet* prevJointSet = link->otherJointSet(curJointSet);
		int cposX = prevJointSet->posX + dir.first;
		int cposY = prevJointSet->posY + dir.second;


		for (int i = 0; i < curJointSet->restrictions.size(); ++i) {
			if (!curJointSet->restrictions[i].check(cposX, cposY)) return false;
		} //preveri ali so vse omejitve ok

		if (curJointSet->embeded) {
			if (cposX != curJointSet->posX || cposY != curJointSet->posY) return false;
		}

		if (!checkNetPos(prevJointSet, curJointSet, cposX, cposY, link, relMatrix)) {
			return false;
		}

		if (curJointSet->embeded) {
			if (cposX == curJointSet->posX && cposY == curJointSet->posY) return true;
		}

		curJointSet->posX = cposX;
		curJointSet->posY = cposY;

		JointSet* jointSetOnPos = graph.getJointSet(net.findJointSet(cposX, cposY));
		if (jointSetOnPos != nullptr) {
			if (!joinJointSets(curJointSet, jointSetOnPos, relMatrix)) {
				return false;
			}
			JointSet* tmp = jointSetOnPos;
			jointSetOnPos = curJointSet;
			curJointSet = tmp;
		}
		else if (!embed(curJointSet, cposX, cposY, relMatrix)) {// vstavi ta joint, za vstavitev linkov poskrbi ta funcija
			return false; 
		}

		bool isOk = true;
		for (int i = 0; i < curJointSet->conLinksN;++i) {
			isOk = false;
			Link* nextLink = curJointSet->conLinkAt(i);
			JointSet* nextJointSet =nextLink->otherJointSet(curJointSet);
			if (nextLink != link && nextLink != link->boundPair) {
				for (int j = 0; j < allDirections.size(); ++j) {
					if (tryEmbedLinkDir(nextLink, nextJointSet, allDirections[j], relMatrix)) {
						isOk = true;
						break;
					}
				}
			}
			else isOk = true;
			if (!isOk) break;
		}//moramo vstavit vse naslednje linke

		if (jointSetOnPos != nullptr) {
			separateJointSets(curJointSet, jointSetOnPos);
		}
		else {
			unembed(curJointSet);
		}
		return isOk;
		
	}

	void findEffectedHalfFixedLinks_embed(list<Link*>* effectedLinks, JointSet* embededJS) {
		graph.linkVisitID++;
		for (list<Link*>::iterator it = effectedLinks->begin(); it != effectedLinks->end(); ++it) {
			(*it)->visitID = graph.linkVisitID;
		}

		for (int x = -2; x <= 2; ++x) {
			for (int y = -2; y <= 2; ++y) {
				if (manhattanDistance(x, y, 0, 0) > EFFECTED_AREA) continue;
				int cJointSetID = net.findJointSet(x + embededJS->posX, y + embededJS->posY);
				if (cJointSetID == -1) continue;
				JointSet* cJointSet = graph.getJointSet(cJointSetID);

				for (int j = 0; j < cJointSet->conLinksN; ++j) {  //za vsak link v tem jointu
					Link* cLink = cJointSet->conLinkAt(j);

					if (cLink->visitID < graph.linkVisitID) {
						cLink->visitID = graph.linkVisitID;
						if (cLink->boundPair != nullptr) cLink->boundPair->visitID = graph.linkVisitID;
						if (cLink->pJoint->embeded() != cLink->nJoint->embeded()) effectedLinks->push_back(cLink);
					}
				}
			}
		}
	}

	bool testEmbed(RelMatrix* relMatrix, list<Link*>* linksToCheck) {
		for (list<Link*>::iterator it = linksToCheck->begin(); it != linksToCheck->end(); ) {
			if ((*it)->nJoint->embeded() == (*it)->pJoint->embeded()) {
				it = linksToCheck->erase(it);
				continue;
			}


			int i = 0;
			JointSet* freeJointSet = (*it)->nJoint->embeded() ? (*it)->pJoint->jointSet : (*it)->nJoint->jointSet;
			while (i < (*it)->directions.size()) {
				if (tryEmbedLinkDir(*it, freeJointSet, (*it)->directions[i], relMatrix)) {
					++i;
				}
				else (*it)->directions.erase((*it)->directions.begin() + i);
			}

			if ((*it)->directions.size() == 0) return false;
			if (!fixedSpace && (*it)->directions.size() == 2 && (*it)->directions[0].first == 0 && (*it)->directions[1].first == 0) {
				(*it)->directions.erase((*it)->directions.begin() + 1);
				fixedSpace=true;
				//fiksiramo prostor, izberemo direction (0,1) (samo prvič)
				revertStack.back().push_back(Revert());
			}
			if ((*it)->directions.size() == 1) {
				int x = (*it)->otherJointSet(freeJointSet)->posX + (*it)->directions[0].first;
				int y = (*it)->otherJointSet(freeJointSet)->posY + (*it)->directions[0].second;
				JointSet* jointSetOnPos = graph.getJointSet(net.findJointSet(x, y));
				if (jointSetOnPos != nullptr) {
					joinJointSets(freeJointSet, jointSetOnPos, relMatrix);
					revertStack.back().push_back(Revert(jointSetOnPos, freeJointSet));
					freeJointSet = jointSetOnPos;
				}
				else {
					embed(freeJointSet, x, y, relMatrix); 
					revertStack.back().push_back(Revert(freeJointSet));
				}//actually embed stuff

				linksToCheck->erase(it); //remove embeded link
				findEffectedHalfFixedLinks_embed(linksToCheck, freeJointSet); //freeJointSet je sedaj embeded, zato pogledamo posledice v točkav v okolici

				//new restriction function in graph
				graph.handleNewRestriction(freeJointSet);

				return testEmbed(relMatrix, linksToCheck); //stanje v mreži se je spremenilo, začni preverjat vse od začetka (morebitni novi embedingi)

			}
			
			++it;
		}
		return true;
	}
	
	bool connectAndEmbed(Link* link1, Link* link2, RelMatrix* relMatrix) {//link1=linkToConnect

		bool paralel = relMatrix->connectsParalel(link1);
		JointSet* l1pJointS = link1->pJoint->jointSet;
		JointSet* l1nJointS = link1->nJoint->jointSet;
		JointSet* l1pJointSPair = (paralel ? link2->pJoint->jointSet : link2->nJoint->jointSet);
		JointSet* l1nJointSPair = (paralel ? link2->nJoint->jointSet : link2->pJoint->jointSet);

		revertStack.push_back(list<Revert>());

		//4 jointe združimo v 2
		if (!joinJointSets(l1pJointS, l1pJointSPair, relMatrix)) return revertConnectAndEmbed(relMatrix);
		revertStack.back().push_back(Revert(l1pJointS->inUse ? l1pJointS : l1pJointSPair, l1pJointS->inUse ? l1pJointSPair : l1pJointS));
		if (!joinJointSets(l1nJointS, l1nJointSPair, relMatrix)) return revertConnectAndEmbed(relMatrix);
		revertStack.back().push_back(Revert(l1nJointS->inUse ? l1nJointS : l1nJointSPair, l1nJointS->inUse ? l1nJointSPair : l1nJointS));
		//^^^^^^^^connect

		int cDist = min(min(l1pJointS->centerDistance, l1nJointS->centerDistance), min(l1pJointSPair->centerDistance, l1nJointSPair->centerDistance));
		if (!checkLinkPairNumberInArea(net.findJointSet(0, 0), cDist, relMatrix)) {
			return revertConnectAndEmbed(relMatrix);
		}
	
		
		list<Link*>* halfFixedLinks = graph.findEffectedHalfFixedLinks_connect(link1);
		//najdemo vse linke z enim vloženim jointom
		graph.findRestrictions();
		//na novo najdemo vse omejitve
		if (testEmbed(relMatrix, halfFixedLinks)) {
			delete halfFixedLinks;
			return true;
		}
		else{
			delete halfFixedLinks;
			return revertConnectAndEmbed(relMatrix);
		}

		//tests if embeding is possible + embeds if only 1 option + joins joint sets if on same position
	}
	//KOPIRAJ IN POPRAVI RESTRICTIONS NAMESTO PONOVNEGA RAČUNANJA, 
	//RESTRICTIONS SE SPREMENIJO OB CONNECT IN OB VLOŽITVI

	bool checkStability() {
		//postavitev verig je v iskani obliki (ker vlagamo v kvadradno mrežo)
		//3. pogoj stabilnosti je zadovoljen, saj novo verigo ob dodajanju takoj povežemo


		for (int x = -const_radius + 1; x < const_radius; ++x) { //walking  throught diamond shape, mogoče še zmanjšati območje  v primerjavi z r
			//mogoče zmanjšati na kvadrat?
			int yRange = (const_radius - 1) - abs(x);
			for (int y = -yRange; y <= yRange; ++y) {
				int jointSetID = net.findJointSet(x,y);
				if (jointSetID == -1) return false;
				JointSet* jointSet = graph.getJointSet(jointSetID);
				if (graph.countLinkNumberInJointSet(jointSet) != 4) return false; 
				//s tem smo preverili 1. pogoj stabilnosti. Ker smo znotraj r so vsi linki povezani
				//joint mora biti vložen in imeti 4 povezane linke okoli sebe

				if (!graph.checkCon2Stability(jointSet)) return false;
				//preverimo ali smo s prejšnjimi in naslednjimi členi prišli do vseh sosednjih vozlišč
			}
		}
		return true;
	}

	//zapiši pogoj ekvivalentnosti formalno

	ChainStructure(RelMatrix* relMatrix) {
		fixedSpace = false;
		net = Net();
		graph = Graph();

		Link* link = graph.addChain(0, const_k / 2 - 1);
		embed(link->pJoint->jointSet, 0, 0, relMatrix);
		embed(link->nJoint->jointSet, 1, 0, relMatrix);
	}
};

class SolutionStructure {
public:
	Net net;
	vector<Link> links;
	bool solutionExists;

	int simTypes[8][2][2] = {
		{ {1,0},{0,1} },
		{ {1,0},{0,-1} },
		{ {0,1},{-1,0} },
		{ {0,-1},{-1,0} }, 
		{ {-1,0},{0,-1} }, 
		{ {-1,0},{0,1} }, 
		{ {0,-1},{1,0} }, 
		{ {0,1},{1,0} } };

	SolutionStructure() {
		solutionExists = false;
	}

	void printSol(ofstream* proofF) {
		for (int x = -const_radius + 1; x < const_radius; ++x) { //walking  throught diamond shape, mogoče še zmanjšati območje  v primerjavi z r
			int yRange = (const_radius - 1) - abs(x);
			for (int y = -yRange; y <= yRange; ++y) {
				*proofF << "x:" << x << " y:" << y << ", JointSet:" << net.findJointSet(x, y) << ", Links:";
				*proofF << " up:(" << net.findLinksOn(x, y, x , y + 1).first << "," << net.findLinksOn(x, y, x, y + 1).second<<")";
				*proofF	<< " right:(" << net.findLinksOn(x, y, x+1, y).first << "," << net.findLinksOn(x, y, x + 1, y).second << ")";
				*proofF << " down:(" << net.findLinksOn(x, y, x , y-1).first << "," << net.findLinksOn(x, y, x , y-1).second << ")";
				*proofF << " left:(" << net.findLinksOn(x, y, x - 1, y).first << "," << net.findLinksOn(x, y, x - 1, y).second << ")";
				*proofF << endl;
			}
		}
		*proofF << endl << "LINKS:"<<endl;
		for (int i = 0; i < links.size(); ++i) {
			*proofF <<"id:"<<i<<", ChainType:"<<links[i].chainType<<", LinkType"<<links[i].linkType<< endl;
		}
	}

	void saveSolution(ChainStructure* solution) { //shranimo net in podatke o linkih,
												  //pointerji ne bodo delovali, jih ne rabimo
		net = solution->net;
		for (int i = 0; i < solution->graph.links.size(); ++i) {
			links.push_back(*solution->graph.links[i]);
		}
		solutionExists = true;
	}

	pair<int, int> transform(pair<int, int> vertex, int simType, pair<int,int> shift) {
		//A*point+shift
		int x = simTypes[simType][0][0] * vertex.first + simTypes[simType][0][1] * vertex.second + shift.first;
		int y = simTypes[simType][1][0] * vertex.first + simTypes[simType][1][1] * vertex.second + shift.second;
		return make_pair(x, y);
	}

	vector < pair<int, int>> makeNeighbours(pair<int,int> vertex) {
		int dx = vertex.first;
		int dy = vertex.second;
		vector<pair<int, int>> neighbours;
		neighbours.push_back(make_pair(dx + 1, dy));
		neighbours.push_back(make_pair(dx, dy - 1));
		neighbours.push_back(make_pair(dx - 1, dy));
		neighbours.push_back(make_pair(dx, dy + 1));
		return neighbours;
	}

	vector<pair<int, int>>* makeDirections(vector<pair<int, int>>* linkPairs) {
		//določi smeri
		//vsak link ima ali prejšnji link, ali naslednji okoli centralne točke ali pa je robni link
		vector <pair<int, int>>* dirs = new vector<pair<int, int>>;
		for (int i = 0; i < linkPairs->size(); ++i) {
			dirs->push_back(make_pair(LP_UNDEFINED, LP_UNDEFINED));
			for (int j = 0; j < linkPairs->size(); ++j) {
				if (i != j) {
					if ((*linkPairs)[i].first%const_k != 0 && 
						((*linkPairs)[i].first == (*linkPairs)[j].first + 1 || (*linkPairs)[i].first == (*linkPairs)[j].second + 1)) {
							(*dirs)[i].first = LP_OUT;
					}
					if ((*linkPairs)[i].first%const_k != const_k - 1 &&
						((*linkPairs)[i].first == (*linkPairs)[j].first - 1 || (*linkPairs)[i].first == (*linkPairs)[j].second - 1)) {
							(*dirs)[i].first = LP_CENTER;
					}
					if ((*linkPairs)[i].second%const_k != 0 &&
						((*linkPairs)[i].second == (*linkPairs)[j].first + 1 || (*linkPairs)[i].second == (*linkPairs)[j].second + 1)) {
							(*dirs)[i].second = LP_OUT;
					}
					if ((*linkPairs)[i].second%const_k != const_k - 1 &&
						((*linkPairs)[i].second == (*linkPairs)[j].first - 1 || (*linkPairs)[i].second == (*linkPairs)[j].second - 1)) {
							(*dirs)[i].second = LP_CENTER;
					}
				}
			}
		}
		for (int i = 0; i < linkPairs->size(); ++i) {
			if ((*dirs)[i].first == LP_UNDEFINED) {
				if ((*linkPairs)[i].first%const_k == 0) (*dirs)[i].first = LP_OUT;
				else (*dirs)[i].first = LP_CENTER;
			}
			if ((*dirs)[i].second == LP_UNDEFINED) {
				if ((*linkPairs)[i].second%const_k == 0) (*dirs)[i].second = LP_OUT;
				else (*dirs)[i].second = LP_CENTER;
			}
		}
		return dirs;
	}

	bool checkEquivalencyPoint(ChainStructure* newCS, pair<int, int> shift, int simType, pair<int,int> CSVertex, pair<int,int> solVertex) {
		int dx = CSVertex.first;
		int dy = CSVertex.second;

		int solx = solVertex.first;
		int soly = solVertex.second; //točka v this, ki mora biti enaka

		vector<pair<int, int>> neighboursCS = makeNeighbours(CSVertex); 
		
		//definicija kontejnerjev za informacije
		vector < pair<int, int>> neighboursSol;
		vector < pair<int, int>> linksCS;
		vector < pair<int, int>> linksSol;
		vector<pair<int, int>>* dirCS; //does link point towards center
		vector<pair<int, int>>* dirSol;

		for (int i = 0; i < neighboursCS.size(); ++i) {
			//pridobivanje vseh informacij
			neighboursSol.push_back(transform(neighboursCS[i], simType, shift));
			linksCS.push_back(newCS->net.findLinksOn(dx, dy, neighboursCS[i].first, neighboursCS[i].second));
			linksSol.push_back(net.findLinksOn(solx, soly, neighboursSol[i].first, neighboursSol[i].second));
		}

		dirCS = makeDirections(&linksCS);
		dirSol = makeDirections(&linksSol);

		//i-ti neighbourCS ustreza i-temu paru linksCS, ki ustreza (s simetrijo in premikom) i-temu paru linksSol. Podobno z dirCS in dirSol

		for (int i = 0; i < linksCS.size() * 2; ++i) {//za vsak link v vozlišču
			int it = i / 2;
			int linkID = (i % 2) ? linksCS[it].second : linksCS[it].first;
			int dir = (i % 2) ? (*dirCS)[it].second : (*dirCS)[it].first;
			Link* link = newCS->graph.getLink(linkID); //<-ta link

			//preveri match linkov, zaradi pogoja v matriki ne moremo na isti povezavi imeti dva enaka linka, zato vedno najdemo le enega,
			//ki je enak po vrsti in usmerjenosti
			int linkSol; 
			if (link->chainType == links[linksSol[it].first].chainType 
				&& link->linkType == links[linksSol[it].first].linkType 
				&& dir == (*dirSol)[it].first)
				linkSol = linksSol[it].first;
			else if (link->chainType == links[linksSol[it].second].chainType 
				&& link->linkType == links[linksSol[it].second].linkType 
				&& dir == (*dirSol)[it].second)
				linkSol = linksSol[it].second;
			else return false;
			//linkSol je ekvivalentni link
			
			//preveri pravilnost nadaljevanja
			for (int j = 0; j < linksCS.size(); ++j) {
				if (linkID % const_k != const_k-1 && (linksCS[j].first == linkID + 1 || linksCS[j].second == linkID + 1)) {
					//link ima nadaljevanje na j-ti povezavi, mora imeti linkSol nadaljevanje na projekciji j-te povezave
					if (linksSol[j].first != linkSol + 1 && linksSol[j].second != linkSol + 1) return false;
				}
				if (linkID % const_k != 0 && (linksCS[j].first == linkID - 1 || linksCS[j].second == linkID - 1)) {
					if (linksSol[j].first != linkSol - 1 && linksSol[j].second != linkSol - 1) return false;
				}
			}
		}

		return true;
	}

	bool checkEquivalencyShiftSim(ChainStructure* newCS, pair<int,int> shift, int simType) { 
		//preveri ali sta strukturi enaki če točka 0,0 newCS leži na shift = (x,y) this->net z določeno simetrijo
		for (int dx = -const_radius + 1; dx < const_radius; ++dx) { //walking  throught diamond shape, mogoče še zmanjšati območje  v primerjavi z r
			//mogoče zmanjšati na kvadrat?
			int yRange = (const_radius - 1) - abs(dx);
			for (int dy = -yRange; dy <= yRange; ++dy) {
				pair<int, int> CSVertex = make_pair(dx,dy);
				pair<int, int> solVertex = transform(CSVertex, simType, shift);

				if (manhattanDistance(dx, dy, 0, 0) < const_radius && manhattanDistance(solVertex.first, solVertex.second, 0, 0) < const_radius) {//sledi, da so linki <= const_radius
					if (!checkEquivalencyPoint(newCS, shift, simType, CSVertex, solVertex)) {
						return false;
					}
				}
			}
		}
		return true;
	}

	bool checkEquivalency(ChainStructure* newCS) {//preveri ali sta strukturi enaki (premiki, rotacije)
		//funkcija se sprehodi skozi točke premika in kliče funkcijo s premikom;
		for (int x = -const_radius + 1; x < const_radius; ++x) { //walking  throught diamond shape, mogoče še zmanjšati območje  v primerjavi z r
			int yRange = (const_radius - 1) - abs(x);
			for (int y = -yRange; y <= yRange; ++y) {
				for (int simType = 0; simType < 8; ++simType) {
					if (checkEquivalencyShiftSim(newCS, make_pair(x, y), simType)) {
 						return true;
					}
				}
			}
		}
		return false;
	}
};

/*
.-.-.-.-.     r=3 e=1
^	   ^ zadnji link, ki mora biti povezan
0   ^ zadnji joint, za katerega se preverja stabilnost, v tem jointu se tudi preverja ekivalenca
	 ^ zadnji link, za katerega se preverja ekvivalenca
*/
int evaluateRelationMatrix(RelMatrix* relMatrix, ChainStructure* chainStructure, SolutionStructure* solStruct, int recDepth, ofstream* proofF) {
	/*
	Preveri ali lahko z dodajanjem fizično možnih povezav med linki v graph glede na relationMatrix dobimo stabilno postavitev,
		ki zapolni minimalni vzorec(preizkovalni prostor).
	Omejitve podatkov :
		relationMatrix ustreza pogojem, ki smo jih postavili za relacijsko matriko 2 * k*n x 2 * k*n(ena enica v vsaki vrstici / stoplcu, simetrije)
		chainStrukture je struktura delno vloženega/povezanega grafa linkov(ki predstavljajo člene) in jointov, ki ustreza fizičnim omejitvam v kvadratni mreži
		Vrne :
	-ambigous(matrika vodi v dve ali več fizično možnih postavitev, ki zapolnijo minimalni vzorec
	-unstable (našli smo zapolnitev minimalnega vzorca, ki ne ustreza pogojem stabilnosti
	-impossible (graph ne vodi v fizično možno postavitev pri zapolnitvi vseh povezav)
	-solutionfound (z dodajanjem povezav v grah pridemo do 1 postavitve, ki je fizično možna, zapolni preizkovalni prostor in je stabilna)
	-return_to_radius (solutionfound + vrnemo se v notranje območje)*/
	if (chainStructure == nullptr) {
		chainStructure = new ChainStructure(relMatrix);
	}
	
	Link* linkToConnect = chainStructure->findLinkToConnect(relMatrix);
	//najbližji link centru, ki ni povezan in je na razdalji manjši kot const_r+const_e

	if (linkToConnect == nullptr) { //vsi linki do razdalje const_r+const_e povezani
		chainStructure->graph.printGraph();
		if (!chainStructure->checkStability()) {
			chainStructure->graph.printGraph(proofF);
			return UNSTABLE;
		}
		//zavrži matriko, saj v stanju najmanjše proste energije ni v dobri postavitvi

		if (!solStruct->solutionExists) {
			solStruct->saveSolution(chainStructure);
			return SOLUTIONFOUND; //vrnemo se za območje e, TODO: define
		}

		if (solStruct->checkEquivalency(chainStructure)) {
			return SOLUTIONFOUND;  //vrnemo se za območje e, , TODO: define
		}
		
		chainStructure->graph.printGraph(proofF);
		*proofF << "____ OTHER SOLUTION" << endl << endl;
		solStruct->printSol(proofF);
		return AMBIGOUS; // ambigous
	}

	vector<Link*>* candidates = chainStructure->conCandidates(linkToConnect, relMatrix); 
	//vsi prosti linki, ki so povezljivi z linkToConnect glede na relMatrix
	//in ustrezajo osnovnim pogojem (razdalja med candidate_i in linkToConnect, vloženost)

	bool isTerminating = true;
	bool foundSolution = false;
	int retVal = IMPOSSIBLE;
	int cd = linkToConnect->centerDistance();
	for (int i = 0; i < candidates->size() + 1; ++i) {//+1 ker je možno dodati novo verigo

		Link* candidate;
		if (i < candidates->size()) candidate = (*candidates)[i];
		else candidate = chainStructure->addCounterChain(relMatrix, linkToConnect);

	

		DEBUGPRINT("Povezujemo link: %d with: %d  on:%d\n", linkToConnect->id, candidate->id, recDepth);

		if (chainStructure->connectAndEmbed(linkToConnect, candidate, relMatrix)) {
			//chainStructure->graph.printGraph();
			retVal = evaluateRelationMatrix(relMatrix, chainStructure, solStruct, recDepth+1, proofF);
			chainStructure->revertConnectAndEmbed(relMatrix);
			isTerminating = false;
		}
		else {
			retVal = IMPOSSIBLE;
		}

		if (i == candidates->size()) chainStructure->graph.removeLastChain(); 
		//vedno odstranjujemo samo zadnjo verigo, ki smo jo dodali (če smo jo dodali)
		//ta sedaj ni povezana z drugimi, torej le popamo konce vektorjev

		if (retVal == SOLUTIONFOUND) {
			if (linkToConnect->centerDistance() >= const_radius - 1) return SOLUTIONFOUND;
			else foundSolution = true;
		} //found solution + gremo iz rekurzije še retVal korakov
		
		else if (retVal == AMBIGOUS || retVal == UNSTABLE) return retVal; //ambigous || unstable
		//če ne velja nič od zgornjega je impossible in nadaljujemo
	}

	if (isTerminating) {
		linkToConnect->nJoint->jointSet->terminateCount += 1;
		linkToConnect->pJoint->jointSet->terminateCount += 1;
	}

	delete candidates;
	return foundSolution ? SOLUTIONFOUND : IMPOSSIBLE;
}

int outf(int r) {
	cout << endl;
	int a;
	cin >> a;
	return r;
}

int main()
{
	DEBUGPRINT("n:%d   k:%d  search size:  edge: \n\n", const_n, const_k, const_radius, const_extend);
	int matrixn = 19;

	ofstream results;
	results.open("results/results.txt");
	
	for (int i = 0; i < matrixn; ++i) {
		RelMatrix* relMatrix = new RelMatrix();
		ofstream proof;
		proof.open("results/proof/proof" + to_string(i) + ".txt");
		if (relMatrix->readRelMatrix("matrices/i"+to_string(i)+".txt")) DEBUGPRINT("COMPLETE relation Matrix read. \n -----------  \n \n");
		else {
			DEBUGPRINT("FAIL relation Matrix read. \n -----------  \n \n");
			outf(0);
		}
		relMatrix->transformRelMatrix();
		clock_t time = clock();
		int retVal = evaluateRelationMatrix(relMatrix, nullptr, new SolutionStructure(), 0, &proof);
		
		proof.close();
		

		results << "i" << i << " - ";

		if (retVal == AMBIGOUS) results << "AMBIGOUS  ";
		if (retVal == UNSTABLE) results << "UNSTABLE  ";
		if (retVal == IMPOSSIBLE) results << "IMPOSSIBLE  ";
		if (retVal == SOLUTIONFOUND) results << "SOLUTIONFOUND  ";

		results << clock() - time << endl;
	}

	results.close();
	outf(0);
}