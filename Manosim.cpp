#include <graphics.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
using namespace std;

#define slowTime 0.5
#define fastTime 0.1
#define fileName ("add2.as")

int org, AR, DR, PC; //Registers
int accumulator;
int delayTime;
bool error = false, keyPress = false;
string IR, instruction, message;
int signFlag=0,zeroFlag=0,carryFlag=0,parityFlag = 0;
vector<string> lines, preHalt, postHalt; //Preprocessing
map<string,int> valueOf, address; //address["A"] -> Gives memory address of A, valueOf["A"] -> Gives value of A
map<int,string> variables, hexmap, instructionAtAddress; //variables[100] -> Variable stored at that address
multimap<string,int> memory;
set<pair<int,string> > memorySet; //Contains (address,register) pair
ifstream fin(fileName); //Read from this file

void process(string,string);
void loadFromAcc(string);
void load(int);
void originate(string);
void add(int);
void subtract(int);
void multiply(int);
void divide(int);
void bitwiseXOR(int);
void bitwiseOR(int);
void bitwiseAND(int);
void complement();
void increment();
void clearAccumulator();
int hexToInt(string);
string hexValue(int);
void createHexMap();
void readLines();
void separateByHalt();
void processPostHalt();
void processPreHalt();
void generateInstructionAtAddressMap();
//void generateInstructionAtAddress();
void printVariables();
void printHexMap();
void callProcess();
void setZeroAndSignFlags();
void setParityFlag();
void setCarryFlag();
void setFlags();
void processGraphics();
void initiateGraphics();
void video();
template <class t> string display(t,int=4);
void prompt();

//display(hexValue(variable)) -> Displays the hexadecimal value of variable in correct format

int main()
{
    prompt(); //Prompts user for the speed of execution
    readLines(); //Read all lines and store in 'lines' vector, ignoring comments
    separateByHalt(); //Stores position of "HLT" line and separates both pre-halt and post-halt vectors
    processPostHalt(); //Stores different mappings and adds to memory multi-map
    processPreHalt(); //Updates memory multi-map, instructions can be non-unique
    createHexMap(); //Create HexMap
    generateInstructionAtAddressMap();
    //generateInstructionAtAddress();
    //printVariables();
    //printHexMap(); //Print Hexmap
    initiateGraphics(); //Create graphic window
    callProcess();
    processGraphics();
    //cout << "Final value of accumulator " << hexValue(accumulator);
    getch();
}

void prompt()
{
    cout << "Welcome! How fast would you like the results to appear?" << endl;
    cout << "1. On key press" << endl;
    cout << "2. Slow." << endl;
    cout << "3. Fast." << endl;
    cout << "Enter choice -> ";
    int choice; cin >> choice;
    if(choice==1)
        keyPress = true;
    else if(choice==2)
        delayTime = slowTime * 1000;
    else
        delayTime = fastTime * 1000;
}

template <class t>
string display(t n, int w=4)
{
    stringstream ss;
    ss << setw(w) << setfill('0') << n;
    return ss.str();
}

void initiateGraphics()
{
    int gd=DETECT,gm;
    initgraph(&gd,&gm,"");
    initwindow(1200,700);
    processGraphics();
}

void processGraphics()
{
    setfillstyle(SOLID_FILL,WHITE);
    floodfill(1,1,WHITE);
    setfillstyle(SOLID_FILL,WHITE);
    setbkcolor(WHITE);
    setcolor(BLACK);
    outtextxy(900,680,"Made By-Kaushtubh,Emin,Siddhant,Himanshu");
}

void callProcess()
{
    for(int i = 0;i<preHalt.size(); i++)
    {
        video(); //CALL VIDEO FUNCTION
        string s = preHalt[i];
        instruction = s;
        AR = PC;
        video();
        //cout << "PROGRAM COUNTER -> " << hexValue(PC) << endl;
        //cout << "ADDRESS REGISTER -> " << hexValue(AR) << endl;
        PC++;
        message = "";
        IR = hexmap[AR];

        video(); //CALL VIDEO FUNCTION
        //cout << "INSTRUCTION REGISTER -> " << display(IR) << endl;
        int space = s.find(' ');
        string currentInstruction = s.substr(0,space);
        if(space == string::npos) //space not found, CMA etc
            process(currentInstruction,"");
        else
        {
            string varName = s.substr(space+1);
            AR = address[varName];
            message = "AR Updated!";
            video();
            if(currentInstruction!="STA")
            {
                DR = valueOf[varName];
                message = "DR Updated!";
                video();
            }
            //cout << "AR Updated! -> " << display(hexValue(AR)) << endl;
            //cout << "DR Updated! -> " << display(hexValue(DR)) << endl;
            process(currentInstruction, varName);
        }
        if(error)
        {
              message = "Snytax Error!";
              video();
              return;
        }
        //cout << endl;
    }

    //cout << "PROGRAM COUNTER -> " << display(hexValue(PC)) << endl;
    //cout << "AR  -> " << display(hexValue(AR),3) << endl << "IR -> " << IR << endl;
    message="Thank you for using.";
    video();
    outtextxy(460,162,"AR");
    line(415,190,590,190);
}

void setZeroAndSignFlags()
{
    if(accumulator<0)
    {
        zeroFlag = 0;
        signFlag = 1;
        message = "Sign flag set.";
    }
    else if(accumulator == 0)
    {
        zeroFlag = 1;
        signFlag = 0;
        message = "Zero flag set.";
    }
    else
    {
        zeroFlag = 0;
        signFlag = 0;
    }
}

void setCarryFlag()
{
    if(accumulator>=65536)
    {
        carryFlag = 1;
        accumulator %= 65536;
        message = "Carry flag set!";
    }
    else
        carryFlag = 0;
}

void printHexMap()
{
    cout << endl << "HexMap" << endl;
    for(map<int,string>::iterator kv=hexmap.begin();kv!=hexmap.end();kv++)
        cout << hexValue(kv->first) << " -> " << display(kv->second) << endl;
}

void printVariables()
{
    cout << endl << "Variables" << endl;
    for(map<int,string>::iterator p=variables.begin();p!=variables.end();p++)
        cout << hexValue(p->first) << " -> " << p->second << endl;
}

/*
void generateInstructionAtAddress()
{
    cout << "Memory Diagram :" << endl;
    for(set<pair<int,string> >::iterator p=memorySet.begin();p!=memorySet.end();p++)
    {
        int key = p->first;
        string value = p->second;
        instructionAtAddress[key] = value;
        //cout << value << " -> " << hexValue(key) << endl;
    }
}
*/
void generateInstructionAtAddressMap()
{
    for(multimap<string,int>::iterator kv = memory.begin();kv!=memory.end();kv++)
    {
        int key = kv->second + org;
        string value = kv->first;
        instructionAtAddress[key] = value;
        //pair<int,string> p = make_pair(key,value);
        //memorySet.insert(p);
    }
}

void processPreHalt()
{
    int i=0;
    for(int i = 0;i<preHalt.size(); i++)
    {
        string s = preHalt[i];
        int space = s.find(' ');
        string instruction = s.substr(0,space);
        memory.insert(make_pair(instruction,i));
    }
}

void processPostHalt()
{
    int halt = memory.find("HLT")->second;
    for(int i = 0 ; i<postHalt.size();)
    {
        string s = postHalt[i];
        int comma = s.find(',');
        int firstSpace = s.find(' ');
        int lastSpace = s.rfind(' ');
        string var = s.substr(0,comma);
        string type = s.substr(firstSpace+1,lastSpace-(firstSpace+1));
        string value = s.substr(lastSpace+1);
        stringstream oss(value);
        int x;
        oss >> x;
        valueOf[var] = x;
        //memory[var] = memory["HLT"] + (++i);

        memory.insert(make_pair(var,(halt + (++i))));
        variables[(org+i+halt)] = var;
        address[var] = (org+i+halt);
    }
}

void separateByHalt()
{
    int i=0;
    while(lines[i]!="HLT")
        preHalt.push_back(lines[i++]);

    preHalt.push_back(lines[i]);
    memory.insert(make_pair("HLT",i++)); //memory["HLT"] = i++;

    while(lines[i]!="END")
        postHalt.push_back(lines[i++]);
}

void readLines()
{
    string tempInput;
    while(getline(fin,tempInput))
    {
        if(tempInput.find("ORG")!=string::npos) //This line is ORG
            org = hexToInt(tempInput.substr(tempInput.find(' ')+1));
        else if(tempInput[0]!='/')
            lines.push_back(tempInput);
    }
    PC = org;
}

void createHexMap()
{
    int i = org;
    for(vector<string>::iterator it = preHalt.begin(); it!=preHalt.end(); it++)
    {
        string l = preHalt[it-preHalt.begin()];
        if(l.find(' ')==string::npos) //Single word
        {
            if(l=="CMA")
                hexmap[i] = "7200";
            else if(l=="INC")
                hexmap[i] = "7020";
            else if(l=="CLA")
                hexmap[i] = "7800";
            else if(l=="HLT")
                hexmap[i] = "7001";
        }
        else
        {
            int space = l.find(' ');
            string instruction = l.substr(0,space);
            string postInstruction = l.substr(space+1);
            string value = hexValue(address[postInstruction]);

            if(instruction == "ADD")
                hexmap[i] = "1";
            else if(instruction == "LDA")
                hexmap[i] = "2";
            else if(instruction == "STA")
                hexmap[i] = "3";
            else if(instruction == "SUB")
                hexmap[i] = "A";
            else if(instruction == "MUL")
                hexmap[i] = "B";
            else if(instruction == "DIV")
                hexmap[i] = "C";
            else if(instruction == "XOR")
                hexmap[i] = "D";
            else if(instruction == "OR")
                hexmap[i] = "E";
            else if(instruction == "AND")
                hexmap[i] = "F";

            hexmap[i] += value;
        }
        i++;
    }

    for(vector<string>::iterator it = postHalt.begin(); it!=postHalt.end(); it++)
    {
        string l = postHalt[it-postHalt.begin()];
        int comma = l.find(',');
        int firstSpace = l.find(' ');
        int lastSpace = l.rfind(' ');
        string var = l.substr(0,comma);
        string type = l.substr(firstSpace+1,lastSpace-(firstSpace+1));
        string value = l.substr(lastSpace+1);

        hexmap[i] = hexValue(valueOf[var]);
        i++;
    }
}

int hexToInt(string s)
{
    stringstream ss;
    ss << s;
    int value;
    ss >> hex >> value;
    return value;
}

string hexValue(int n)
{
    stringstream ss;
    ss << uppercase << hex << n;
    return ss.str();
}

void process(string instruction, string postInstruction)
{
    if(instruction == "ORG")
        originate(postInstruction);
    else if(instruction == "LDA")
        load(valueOf[postInstruction]);
    else if(instruction == "STA")
        loadFromAcc(postInstruction);
    else if(instruction == "SUB")
        subtract(valueOf[postInstruction]);
    else if(instruction == "ADD")
        add(valueOf[postInstruction]);
    else if(instruction == "MUL")
        multiply(valueOf[postInstruction]);
    else if(instruction == "DIV")
        divide(valueOf[postInstruction]);
    else if(instruction == "XOR")
        bitwiseXOR(valueOf[postInstruction]);
    else if(instruction == "OR")
        bitwiseOR(valueOf[postInstruction]);
    else if(instruction == "AND")
        bitwiseAND(valueOf[postInstruction]);
    else if(instruction == "INC")
        increment();
    else if(instruction == "CMA")
        complement();
    else if(instruction == "CLA")
        clearAccumulator();
    else if(instruction == "HLT")
        AR = hexToInt("001");
    else
    {
        error = true;
        return;
    }
    setFlags();
}

void setFlags()
{
    setParityFlag();
    setZeroAndSignFlags();
    setCarryFlag();
}

void loadFromAcc(string value)
{
    valueOf[value] = accumulator;
    hexmap[address[value]] = hexValue(valueOf[value]);

}

void load(int value)
{
    accumulator = value;
}

void increment()
{
    accumulator++;
    AR = hexToInt("020");
}

void originate(string value)
{
    org = hexToInt(value);
}

void add(int value)
{
    accumulator += value;
}

void subtract(int value)
{
    accumulator -= value;
}

void clearAccumulator()
{
    accumulator = 0;
    AR = hexToInt("800");
}

void multiply(int value)
{
    accumulator *= value;
}

void divide(int value)
{
    accumulator /= value;
}

void bitwiseXOR(int value)
{
    accumulator ^= value;
}

void bitwiseOR(int value)
{
    accumulator |= value;
}

void bitwiseAND(int value)
{
    accumulator &= value;
}

void complement()
{
    accumulator = ~accumulator;
    AR = hexToInt("200");
}

void setParityFlag()
{
    //Brian Kerningham Algorithm, to count number of set bits in Binary Representation of a number
    int copy = accumulator;
    int count = 0;
    while(copy)
    {
        copy&=(copy-1);
        count++;
    }
    if(count%2==0)
    {
        parityFlag = 1;
        message = "Parity flag set!";
    }
    else
        parityFlag = 0;
}

void video()
{
    cleardevice();
    //Instruction
    int left = 10, right = 600, top = 25, bottom = 100;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(110,50,"INSTRUCTION  :");
    outtextxy(270,50,(char *)instruction.c_str());

    //accumulator

    left=15;right=190;top=150;bottom=225;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(60,162,"Accumulator");
    line(15,190,190,190);
    outtextxy(70,200,(char *)(display(hexValue(accumulator))).c_str());
    //IR

    left=215;right=390;top=150;bottom=225;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(240,162,"Instruction Register");
    line(215,190,390,190);
    outtextxy(240,200,(char *)(display(IR)).c_str());
    //AR
    left=415;right=590;top=150;bottom=225;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(460,162,"Address Register");
    line(415,190,590,190);
    outtextxy(460,200,(char *)(display(hexValue(AR))).c_str());
    //dr
    left=100;right=275;top=275;bottom=350;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(145,287,"Data Register");
    line(100,315,275,315);
    outtextxy(145,325,(char *)(display(hexValue(DR))).c_str());
    //pc
    left=300;right=475;top=275;bottom=350;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    outtextxy(340,287,"Program Counter");
    line(300,315,475,315);
    outtextxy(340,325,(char *)(display(hexValue(PC))).c_str());

    //
    left = 620; right = 880; top = 25; bottom = 690;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    //
    outtextxy(720,30,"MEMORY");
    //outtextxy(670,50,"- - - - - - - - - - -");

    left = 630; right = 870; top = 50; bottom = 680;
    for(int i = 0; i<2; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);

    for(int i = 80; i<680; i+=20)
        line(630,i,870,i);
    line(710,50,710,680);
    line(790,50,790,680);

    outtextxy(645,55,"Address"); outtextxy(730,55,"Value");outtextxy(815,55,"INS");


    int outx = 650, outy = 82;
    char *printadd;
    for(map<int,string>::iterator kv=hexmap.begin();kv!=hexmap.end();kv++)
        {
            string x=(hexValue(kv->first));
            string y=display(kv->second);
            string z=instructionAtAddress[kv->first];
            outtextxy(outx,outy,(char *)x.c_str());
            outtextxy(outx+70,outy,(char *)y.c_str());
            outtextxy(outx+160,outy,(char *)z.c_str());
            outy+=20;
        }
    //assembly code
    left = 930; right = 1180; top = 25; bottom = 675;
    for(int i = 0; i<3; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    line(930,50,1180,50);
    outtextxy(1000,30,"Assembly Code");
    int length_of_code=lines.size();
    int outax=950,outay=60;
    for(int i=0;i<length_of_code;i++)
    {
        string l=lines[i];
        outtextxy(outax,outay,(char *)l.c_str());
        outay+=20;
    }
    //flags
    left = 116; right = 494; top = 420; bottom = 510;
    for(int i = 0; i<5; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    for(int i = 116; i<494; i+=96)
        line(i,420,i,510);
    line(116,465,496,465);
    outtextxy(136,430,"Zero Flag"); outtextxy(232,430,"Sign Flag"); outtextxy(324,430,"Carry Flag"); outtextxy(418,430,"Parity Flag");
    outtextxy(160,480,(char *)(display(hexValue(zeroFlag),1)).c_str());
    outtextxy(250,480,(char *)(display(hexValue(signFlag),1)).c_str());
    outtextxy(340,480,(char *)(display(hexValue(carryFlag),1)).c_str());
    outtextxy(430,480,(char *)(display(hexValue(parityFlag),1)).c_str());


    //message
    left = 20; right = 590; top = 570; bottom = 660;
    for(int i = 0; i<5; i++,left--,right++,top--,bottom++)
        rectangle(left,top,right,bottom);
    setcolor(RED);
    outtextxy(80,602,(char*)("MESSAGE -> "+message).c_str());
    setcolor(BLACK);

    if(keyPress)
        getch();
    else
        delay(delayTime);

    //outtextxy(1060,680,"Made By- CODEGOD");
}
