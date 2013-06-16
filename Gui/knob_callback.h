//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef KNOB_CALLBACK_H
#define KNOB_CALLBACK_H
#include <vector>

class Knob;
class SettingsPanel;
class Node;

class Knob_Callback
{
    SettingsPanel* getSettingsPanel(){return panel;}
public:
    Knob_Callback(SettingsPanel* panel,Node* node);
    ~Knob_Callback();

    
    void addKnob(Knob* knob){knobs.push_back(knob);}
    
    void initNodeKnobsVector();
    
	Node* getNode(){return node;}
    
	void createKnobDynamically();
    
    void removeAndDeleteKnob(Knob* knob);

private:
    Node* node;
    std::vector<Knob*> knobs;
    SettingsPanel* panel;
};

#endif // KNOB_CALLBACK_H
