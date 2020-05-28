#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSTRLENGTH 5
#define TRLENGTH 27

typedef enum {
    false,
    true
} bool;

typedef enum {
    red,
    black
} TreeColor;

// Forward declaration of TRANSITION and STATE structs
struct TRANSITION;
struct STATE;

// Forward declaration of types Transition and State
typedef struct TRANSITION Transition;
typedef struct STATE State;

// Definition of TRANSITION
struct TRANSITION {
    char Read;                      // Read char from memory tape
    char Write;                     // Write char in memory tape
    char HeadMoveDirection;         // Direction where tape head moves
    State * ToState;                // Destination state
    Transition * NextTransition;    // Ptr to next transition IN list
};

// Definition of Transition list.
typedef struct  CHARACTER{
    char Character;                 // Character that indicates what [Write] variable every transition of this list has
    Transition * Transitions;       // List of all transitions that has same [Write] variable
    struct CHARACTER * Next;        // Ptr to next list
} TransitionList;

// Definition of STATE
struct STATE {
    unsigned int id;                         // Name of state
    bool IsAcceptanceState;         // True if this state is an acceptance state (TransitionList is NULL as convention)
    TransitionList * CharacterList;     // List of all state's transitions
};

// Definition of Red-Black Tree node
typedef struct RB {
    State * StatePtr;
    struct RB * p;
    struct RB * left;
    struct RB * right;
    TreeColor color;
} TreeNode;

// Definition of Red-Black Tree
typedef struct {
    TreeNode * root;                // Ptr to tree root
    TreeNode * nil;
} RB_Tree;

typedef struct SYMBOL {
    int BranchID;
	unsigned long int SymbolQty;
	unsigned long int CurrSymbol;
	char Symbol;
    struct SYMBOL * Next;
} Symbol;

// Definition of Memory tape cell
typedef struct CELL {
    Symbol * Symbols;                    // Content of the cell
    struct CELL * Left;                 // Ptr to left cell
    struct CELL * Right;                // Ptr to right cell

} Cell;

typedef struct STACKEL {
    int BranchID;
    struct STACKEL * Next;
    Transition * Trans;
    Cell * MemPositionBuffer;
	unsigned long int CurrSymbolBuffer;
    unsigned long int MovesBuffer;
} StackElem;

// Global variables
Cell * MemoryTape;

Cell * CurrMemPosition;

StackElem * Stack;

RB_Tree * TM;

unsigned long int Moves = 0;

int CurrBranchID = 0;

// Functions
void InitTM();

void ReadInput(char * OutString, int StringSize);

TreeNode * SearchNode(RB_Tree *T, TreeNode * x, unsigned int id);

void InOrderTreeWalk(TreeNode * x);

void LeftRotate(RB_Tree * T, TreeNode * x);

void RightRotate(RB_Tree * T, TreeNode * x);

void TreeInsert(RB_Tree * T, TreeNode * z);

void RBInsertFixup(RB_Tree * T, TreeNode * z);

void SetupTuringMachine();

TreeNode * AddStateToTM(unsigned int id);

void AddTransitionToTM(unsigned int Start,unsigned int End, char Read, char Write, char MemDirection);

void AddTransitionToState(State * TMState, Transition * ToAdd);

void AddTransitionToList(State * TMState, Transition *ToAdd);

Transition * SearchTransition(TransitionList * List, Transition * ToSearch);

TransitionList * SearchReadCharTransList(State * State, char ToSearch);

void SetupAccStatesAndMoves();

void MoveMemHead(char Direction);

Cell * WriteOnTape(Cell * MemCell, char Character);

void StackPush(Transition * Trans);

StackElem * StackPop();

void RunInputs();

void InitStack();

int InitTape();

int RunTM();

void FlushMemorySymbols(int BranchID);

void FreeMemory();

void FreeRightMemory(int BranchID);

void FreeLeftMemory(int BranchID);

void CompressionFixup(Cell * MemCell);

void FreeStack();

void FreeTM();

void FreeTransitions(TreeNode * x);

void FreeNodes(TreeNode * x);

int main() {
    char InstructionCode[INSTRLENGTH] = "";
    MemoryTape = malloc(sizeof(Cell));
    TM = malloc(sizeof(RB_Tree));

    InitTM();

    ReadInput(InstructionCode, INSTRLENGTH);
    if (strcmp(InstructionCode, "tr\n") == 0) {
        SetupTuringMachine();
    } else {
        printf("ERROR: Incorrect input");
    }

    FreeMemory();
    FreeTM();

    return 0;
}

// Read a line of input from stdin and put it into OutString (Mind the \n char!)
void ReadInput(char * OutString, int StringSize) {
    OutString[0] = '\0';
    fgets(OutString, sizeof(char)*StringSize, stdin);
}

// Search function for RB tree
TreeNode * SearchNode(RB_Tree *T, TreeNode * x, unsigned int id) {
    if (x == NULL || x == T->nil || (x->StatePtr != NULL && id == x->StatePtr->id)) {
        return x;
    }

    if (x->StatePtr == NULL) {
        return T->nil;
    }

    if (id < x->StatePtr->id) {
        return SearchNode(T, x->left, id);
    } else {
        return SearchNode(T, x->right, id);
    }
}

void InOrderTreeWalk(TreeNode * x) {
    if (x != TM->nil) {
        InOrderTreeWalk(x->left);

        printf("State nr. %u ; IsAccState: %d\n", x->StatePtr->id, x->StatePtr->IsAcceptanceState);
        TransitionList * CharElem = x->StatePtr->CharacterList;
        while (CharElem != NULL) {
            printf("\tRead [%c]:\n", CharElem->Character);
            Transition * TransElem = CharElem->Transitions;

            while (TransElem != NULL) {
                printf("\t\tWrite: %c ; Move: %c ; ToState: %u\n", TransElem->Write, TransElem->HeadMoveDirection, TransElem->ToState->id);
                TransElem = TransElem->NextTransition;
            }

            CharElem = CharElem->Next;
        }

        InOrderTreeWalk(x->right);
    }
}

// RB tree left rotation
void LeftRotate(RB_Tree * T, TreeNode * x) {
    TreeNode * y = x->right;
    x->right = y->left;

    if (y->left != T->nil) {
        y->left->p = x;
    }

    y->p = x->p;

    if (x->p == T->nil) {
        T->root = y;
    } else if (x == x->p->left) {
        x->p->left = y;
    } else {
        x->p->right = y;
    }
    y->left = x;
    x->p = y;
}

// RB tree right rotation
void RightRotate(RB_Tree * T, TreeNode * x) {
    TreeNode * y = x->left;
    x->left = y->right;

    if (y->right != T->nil) {
        y->right->p = x;
    }

    y->p = x->p;

    if (x->p == T->nil) {
        T->root = y;
    } else if (x == x->p->right) {
        x->p->right = y;
    } else {
        x->p->left = y;
    }
    y->right = x;
    x->p = y;
}

// Insert function for RB tree
void TreeInsert(RB_Tree * T, TreeNode * z) {
    TreeNode * y = T->nil;
    TreeNode * x = T->root;

    while (x != T->nil) {
        y = x;
        if (z->StatePtr->id < x->StatePtr->id) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    z->p = y;

    if (y == T->nil) {
        T->root = z;        // Empty tree
    } else if (z->StatePtr->id < y->StatePtr->id) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->left = T->nil;
    z->right = T->nil;
    z->color = red;

    RBInsertFixup(T, z);
}

// Insert-Fixup function for RB tree
void RBInsertFixup(RB_Tree * T, TreeNode * z) {
    if (z == T->root) {
        T->root->color = black;
    } else {
        TreeNode * x = z->p;

        if (x->color == red) {
            if (x == x->p->left) {
                TreeNode * y = x->p->right;

                if (y->color == red) {
                    x->color = black;
                    y->color = black;
                    x->p->color = red;
                    RBInsertFixup(T, x->p);
                } else {
                    if (z == x->right) {
                        z = x;
                        LeftRotate(T, z);
                        x = z->p;
                    }

                    x->color = black;
                    x->p->color = red;
                    RightRotate(T, x->p);
                }
            } else {
                TreeNode * y = x->p->left;

                if (y->color == red) {
                    x->color = black;
                    y->color = black;
                    x->p->color = red;
                    RBInsertFixup(T, x->p);
                } else {
                    if (z == x->left) {
                        z = x;
                        RightRotate(T, z);
                        x = z->p;
                    }

                    x->color = black;
                    x->p->color = red;
                    LeftRotate(T, x->p);
                }
            }
        }
    }
}

// Initialization of memory tape and TM
void InitTM() {
    MemoryTape->Symbols = NULL;
	MemoryTape->Left = NULL;
	MemoryTape->Right = NULL;
	MemoryTape = WriteOnTape(MemoryTape, '_');

    // Init TM nil node
    TM->nil = malloc(sizeof(TreeNode));
    TM->nil->p = NULL;
    TM->nil->left = NULL;
    TM->nil->right = NULL;
    TM->nil->color = black;
    TM->nil->StatePtr = NULL;

    // Init TM root
    TM->root = malloc(sizeof(TreeNode));
    TM->root->p = TM->nil;
    TM->root->left = TM->nil;
    TM->root->right = TM->nil;
    TM->root->color = black;

    // Init state 0
    TM->root->StatePtr = malloc(sizeof(State));
    TM->root->StatePtr->CharacterList = NULL;
    TM->root->StatePtr->IsAcceptanceState = false;
    TM->root->StatePtr->id = 0;
}

void SetupTuringMachine() {
    char StateInput[TRLENGTH];
    unsigned int StartState, EndState;
    char ReadSymbol, WriteSymbol, MemDirection;

    ReadInput(StateInput, TRLENGTH);

    while (strcmp(StateInput, "acc\n") != 0) {
        sscanf(StateInput, "%u %c %c %c %u\n", &StartState, &ReadSymbol, &WriteSymbol, &MemDirection, &EndState);
        // Add end state of transition to TM (if it doesn't exists)
        AddStateToTM(EndState);
        // Add scanned transition to TM
        AddTransitionToTM(StartState, EndState, ReadSymbol, WriteSymbol, MemDirection);

        ReadInput(StateInput, TRLENGTH);
    }

    SetupAccStatesAndMoves();
}

TreeNode * AddStateToTM(unsigned int id) {
    TreeNode * NewState = SearchNode(TM, TM->root, id);
    if (NewState == TM->nil) {
        NewState = malloc(sizeof(TreeNode));
        NewState->StatePtr = malloc(sizeof(State));

        NewState->StatePtr->id = id;
        NewState->StatePtr->IsAcceptanceState = false;
        NewState->StatePtr->CharacterList = NULL;

        TreeInsert(TM, NewState);
        return NewState;
    } else {
        return NewState;
    }
}

void AddTransitionToTM(unsigned int Start, unsigned int End, char Read, char Write, char MemDirection) {
    TreeNode * NewState = AddStateToTM(Start);
    Transition * NewTransition = malloc(sizeof(Transition));

    // Init new transition to add
    NewTransition->Read = Read;
    NewTransition->Write = Write;
    NewTransition->ToState = SearchNode(TM, TM->root, End)->StatePtr;
    NewTransition->HeadMoveDirection = MemDirection;
    NewTransition->NextTransition = NULL;

    AddTransitionToState(NewState->StatePtr, NewTransition);

}

void AddTransitionToState(State * TMState, Transition * ToAdd) {
    // Check if transition already exists
    if (SearchTransition(TMState->CharacterList, ToAdd) == NULL) {
        AddTransitionToList(TMState, ToAdd);
    }
}


void AddTransitionToList(State * TMState, Transition *ToAdd) {
    if (TMState->CharacterList == NULL) {
        // Allocate memory for a new Character element
        TransitionList * NewCharacterElement = malloc(sizeof(TransitionList));

        // Initialize new char element
        NewCharacterElement->Character = ToAdd->Read;
        NewCharacterElement->Next = NULL;
        NewCharacterElement->Transitions = ToAdd;
        TMState->CharacterList = NewCharacterElement;

    } else {
        TransitionList * CharacterElement = TMState->CharacterList;

        // Find element of CharacterList that matches read character
        while (CharacterElement->Next != NULL && (CharacterElement->Character != ToAdd->Read)) {
            CharacterElement = CharacterElement->Next;
        }

        if (CharacterElement->Next != NULL || (CharacterElement->Character == ToAdd->Read)) {
            // Add [ToAdd] to the head of the transitions list
            ToAdd->NextTransition = CharacterElement->Transitions;
            CharacterElement->Transitions = ToAdd;
        } else {
            // Allocate memory for a new Character element
            TransitionList * NewCharacterElement = malloc(sizeof(TransitionList));

            // Initialize new char element
            NewCharacterElement->Character = ToAdd->Read;
            NewCharacterElement->Next = NULL;
            NewCharacterElement->Transitions = ToAdd;

            CharacterElement->Next = NewCharacterElement;
        }
    }
}

Transition * SearchTransition(TransitionList * List, Transition * ToSearch) {
    TransitionList * CharacterElement = List;

    // Find element of CharacterList that matches read character
    while (CharacterElement != NULL && CharacterElement->Character != ToSearch->Read) {
        CharacterElement = CharacterElement->Next;
    }

    // CharacterElement points to element that has same Character as ToSearch->Read
    if (CharacterElement != NULL) {
        Transition * TransitionElement = CharacterElement->Transitions;

        while (TransitionElement != NULL && (TransitionElement->Write != ToSearch->Write || TransitionElement->ToState != ToSearch->ToState || TransitionElement->HeadMoveDirection != ToSearch->HeadMoveDirection)) {
            TransitionElement = TransitionElement->NextTransition;
        }

        return TransitionElement;

    } else {
        return NULL;
    }

}

TransitionList * SearchReadCharTransList(State * State, char ToSearch) {
    TransitionList * CharacterElement = State->CharacterList;

    while (CharacterElement != NULL && CharacterElement->Character != ToSearch) {
        CharacterElement = CharacterElement->Next;
    }

    return CharacterElement;
}

void SetupAccStatesAndMoves() {
    char AccStateStr[(TRLENGTH-7)/2];     // Max nr. of states
    unsigned int AccState;
    TreeNode * AccNode;

    ReadInput(AccStateStr, (TRLENGTH-7)/2);

    while (strcmp(AccStateStr, "max\n") != 0) {
        sscanf(AccStateStr, "%u\n", &AccState);

        AccNode = SearchNode(TM, TM->root, AccState);
        AccNode->StatePtr->IsAcceptanceState = true;

        ReadInput(AccStateStr, (TRLENGTH-7)/2);
    }

    // Setup max moves
    scanf("%ld\n", &Moves);

    ReadInput(AccStateStr, 6);
    if (strcmp(AccStateStr, "run\n") == 0) {
        RunInputs();
    }
}

// Returns new current state
Cell * WriteOnTape(Cell * MemCell, char Character) {
    if (MemCell->Symbols == NULL) {
		// Can be compressed to the left?
		if (MemCell->Left != NULL && MemCell->Left->Symbols->Symbol == Character) {
			MemCell = MemCell->Left;

			if (MemCell->Symbols->BranchID != CurrBranchID) {
				Symbol * NewMemSymbol = malloc(sizeof(Symbol));
				NewMemSymbol->Symbol = Character;
				NewMemSymbol->BranchID = CurrBranchID;
				NewMemSymbol->SymbolQty = MemCell->Symbols->SymbolQty + 1;
				NewMemSymbol->CurrSymbol = MemCell->Symbols->CurrSymbol + 1;

				NewMemSymbol->Next = MemCell->Symbols;
				MemCell->Symbols = NewMemSymbol;
			} else {
				MemCell->Symbols->SymbolQty++;
				MemCell->Symbols->CurrSymbol++;
			}

			if (MemCell->Right->Right != NULL) {
				Cell * CellRightTmp = MemCell->Right;
				MemCell->Right = MemCell->Right->Right;
				MemCell->Right->Left = MemCell;
				free(CellRightTmp);
			} else {
				free(MemCell->Right);
				MemCell->Right = NULL;
			}

		// Can be compressed to the right?
		} else if (MemCell->Right != NULL && MemCell->Right->Symbols->Symbol == Character) {
			MemCell = MemCell->Right;

			if (MemCell->Symbols->BranchID != CurrBranchID)	{
				Symbol * NewMemSymbol = malloc(sizeof(Symbol));
				NewMemSymbol->Symbol = Character;
				NewMemSymbol->BranchID = CurrBranchID;
				NewMemSymbol->SymbolQty = MemCell->Symbols->SymbolQty + 1;
				NewMemSymbol->CurrSymbol = 1;

				NewMemSymbol->Next = MemCell->Symbols;
				MemCell->Symbols = NewMemSymbol;
			} else {
				MemCell->Symbols->SymbolQty++;
				MemCell->Symbols->CurrSymbol = 1;
			}
			
			if (MemCell->Left->Left != NULL) { 
				Cell * CellLeftTmp = MemCell->Left;
				MemCell->Left = MemCell->Left->Left;
				MemCell->Left->Right = MemCell;
				free(CellLeftTmp);
			} else {
				free(MemCell->Left);
				MemCell->Left = NULL;
			}

		// If cannot be compressed:
		} else {
			MemCell->Symbols = malloc(sizeof(Symbol));
			MemCell->Symbols->Symbol = Character;
			MemCell->Symbols->CurrSymbol = 1;
			MemCell->Symbols->SymbolQty = 1;
			MemCell->Symbols->Next = NULL;
			MemCell->Symbols->BranchID = CurrBranchID;
		}
    } else {
		// If memory head inside series of compressed symbols
		if (MemCell->Symbols->SymbolQty > 1) {
			if (MemCell->Symbols->Symbol != Character) {
				Cell * NewCharCell = malloc(sizeof(Cell));
				NewCharCell->Symbols = malloc(sizeof(Symbol));
				NewCharCell->Symbols->BranchID = CurrBranchID;
				NewCharCell->Symbols->Symbol = Character;
				NewCharCell->Symbols->SymbolQty = 1;
				NewCharCell->Symbols->CurrSymbol = 1;
				NewCharCell->Symbols->Next = NULL;

				// If between last and first symbol of series
				if (MemCell->Symbols->CurrSymbol < MemCell->Symbols->SymbolQty && MemCell->Symbols->CurrSymbol > 1) {
					Cell * NewRestCell = malloc(sizeof(Cell));
					NewRestCell->Symbols = malloc(sizeof(Symbol));
					NewRestCell->Symbols->BranchID = CurrBranchID;
					NewRestCell->Symbols->Symbol = MemCell->Symbols->Symbol;
					NewRestCell->Symbols->SymbolQty = MemCell->Symbols->SymbolQty - MemCell->Symbols->CurrSymbol;
					NewRestCell->Symbols->CurrSymbol = 1;
					NewRestCell->Symbols->Next = NULL;

					Symbol * UpdatedSymbol = malloc(sizeof(Symbol));
					UpdatedSymbol->BranchID = CurrBranchID;
					UpdatedSymbol->Symbol = MemCell->Symbols->Symbol;
					UpdatedSymbol->SymbolQty = MemCell->Symbols->CurrSymbol - 1;
					UpdatedSymbol->CurrSymbol = UpdatedSymbol->SymbolQty;
					
					if (MemCell->Symbols->BranchID != CurrBranchID) {
						UpdatedSymbol->Next = MemCell->Symbols;
						MemCell->Symbols = UpdatedSymbol;
					} else {
						UpdatedSymbol->Next = MemCell->Symbols->Next;
						Symbol * SymbolTmp = MemCell->Symbols;
						MemCell->Symbols = UpdatedSymbol;
						free(SymbolTmp);
					}
					

					NewRestCell->Right = MemCell->Right;
					MemCell->Right = NewCharCell;
					NewCharCell->Right = NewRestCell;
					NewRestCell->Left = NewCharCell;
					NewCharCell->Left = MemCell;

					MemCell = NewCharCell;

				// If last symbol of series
				} else if (MemCell->Symbols->CurrSymbol == MemCell->Symbols->SymbolQty) {
					Symbol * UpdatedSymbol = malloc(sizeof(Symbol));
					UpdatedSymbol->BranchID = CurrBranchID;
					UpdatedSymbol->Symbol = MemCell->Symbols->Symbol;
					UpdatedSymbol->SymbolQty = MemCell->Symbols->CurrSymbol - 1;
					UpdatedSymbol->CurrSymbol = UpdatedSymbol->SymbolQty;
					UpdatedSymbol->Next = MemCell->Symbols;
					
					if (MemCell->Symbols->BranchID != CurrBranchID) {
						MemCell->Symbols = UpdatedSymbol;
					} else {
						free(UpdatedSymbol);
						MemCell->Symbols->SymbolQty--;
						MemCell->Symbols->CurrSymbol--;
					}
					
					NewCharCell->Right = MemCell->Right;
					MemCell->Right = NewCharCell;
					NewCharCell->Left = MemCell;
					
					if (NewCharCell->Right != NULL) {
						NewCharCell->Right->Left = NewCharCell;
					}

					MemCell = NewCharCell;

				// If first symbol of series
				} else if (MemCell->Symbols->CurrSymbol == 1) {
					Cell * NewRestCell = malloc(sizeof(Cell));
					NewRestCell->Symbols = malloc(sizeof(Symbol));
					NewRestCell->Symbols->BranchID = CurrBranchID;
					NewRestCell->Symbols->Symbol = MemCell->Symbols->Symbol;
					NewRestCell->Symbols->SymbolQty = MemCell->Symbols->SymbolQty - 1;
					NewRestCell->Symbols->CurrSymbol = 1;
					NewRestCell->Symbols->Next = NULL;
					
					if (MemCell->Symbols->BranchID != CurrBranchID) {
						// Using only symbol structure of NewCharCell
						NewCharCell->Symbols->Next = MemCell->Symbols;
						MemCell->Symbols = NewCharCell->Symbols;				
					} else {
						NewCharCell->Symbols->Next = MemCell->Symbols->Next;
						Symbol * SymbolTmp = MemCell->Symbols;
						MemCell->Symbols = NewCharCell->Symbols;
						free(SymbolTmp);
					}

					free(NewCharCell);

					NewRestCell->Right = MemCell->Right;
					NewRestCell->Left = MemCell;
					MemCell->Right = NewRestCell;

					if (NewRestCell->Right != NULL) {
						NewRestCell->Right->Left = NewRestCell;
					}
				}
			}

		// Current memory cell has no compressed symbols
		} else {
			// Can be compressed to the Left?
			if (MemCell->Left != NULL && MemCell->Left->Symbols->Symbol == Character) {
				
				// If modifying cell is memory tape reference, update
				if (MemCell == MemoryTape) {
					MemoryTape = MemCell->Left;
				}				
				MemCell = MemCell->Left;

				if (MemCell->Symbols->BranchID != CurrBranchID) {
					Symbol * NewMemSymbol = malloc(sizeof(Symbol));
					NewMemSymbol->Symbol = Character;
					NewMemSymbol->BranchID = CurrBranchID;
					NewMemSymbol->SymbolQty = MemCell->Symbols->SymbolQty + 1;
					NewMemSymbol->CurrSymbol = MemCell->Symbols->CurrSymbol + 1;

					NewMemSymbol->Next = MemCell->Symbols;
					MemCell->Symbols = NewMemSymbol;

				} else {
					MemCell->Symbols->SymbolQty++;
					MemCell->Symbols->CurrSymbol++;
				}

				if (MemCell->Right->Symbols->BranchID == CurrBranchID) {
					Symbol * SymbolTmp = MemCell->Right->Symbols;
					MemCell->Right->Symbols = MemCell->Right->Symbols->Next;
					free(SymbolTmp);
				}
				if (MemCell->Right->Symbols == NULL) {
					Cell * MemTmp = MemCell->Right;
					MemCell->Right = MemCell->Right->Right;

					if (MemCell->Right != NULL) {
						MemCell->Right->Left = MemCell;
					}

					free(MemTmp);
				} else {
					// Use of dummy symbol that has qty = 0 so that it is ignored while running.
					Symbol * DummySymbol = malloc(sizeof(Symbol));
					DummySymbol->BranchID = CurrBranchID;
					DummySymbol->Symbol = '-';
					DummySymbol->SymbolQty = 0;
					DummySymbol->CurrSymbol = 0;
					DummySymbol->Next = MemCell->Right->Symbols;

					MemCell->Right->Symbols = DummySymbol;
				}
			// Can be compressed to the Right?
			} else if (MemCell->Right != NULL && MemCell->Right->Symbols->Symbol == Character) {
				
				// If modifying cell is memory tape reference, update
				if (MemCell == MemoryTape) {
					MemoryTape = MemCell->Right;
				}
				MemCell = MemCell->Right;

				if (MemCell->Symbols->BranchID != CurrBranchID) {
					Symbol * NewMemSymbol = malloc(sizeof(Symbol));
					NewMemSymbol->Symbol = Character;
					NewMemSymbol->BranchID = CurrBranchID;
					NewMemSymbol->SymbolQty = MemCell->Symbols->SymbolQty + 1;
					NewMemSymbol->CurrSymbol = 1;

					NewMemSymbol->Next = MemCell->Symbols;
					MemCell->Symbols = NewMemSymbol;

				} else {
					MemCell->Symbols->SymbolQty++;
					MemCell->Symbols->CurrSymbol = 1;
				}

				if (MemCell->Left->Symbols->BranchID == CurrBranchID) {
					Symbol * SymbolTmp = MemCell->Left->Symbols;
					MemCell->Left->Symbols = MemCell->Left->Symbols->Next;
					free(SymbolTmp);
				}
				if (MemCell->Left->Symbols == NULL) {
					Cell * MemTmp = MemCell->Left;
					MemCell->Left = MemCell->Left->Left;

					if (MemCell->Left != NULL) {
						MemCell->Left->Right = MemCell;
					}

					free(MemTmp);
				} else {
					// Use of dummy symbol that has qty = 0 so that it is ignored while running.
					Symbol * DummySymbol = malloc(sizeof(Symbol));
					DummySymbol->BranchID = CurrBranchID;
					DummySymbol->Symbol = '-';
					DummySymbol->SymbolQty = 0;
					DummySymbol->CurrSymbol = 0;
					DummySymbol->Next = MemCell->Left->Symbols;

					MemCell->Left->Symbols = DummySymbol;
				}

			// Cannot be compressed
			} else {
				if (MemCell->Symbols->BranchID != CurrBranchID) {
					Symbol * NewMemSymbol = malloc(sizeof(Symbol));
					NewMemSymbol->Symbol = Character;
					NewMemSymbol->BranchID = CurrBranchID;
					NewMemSymbol->CurrSymbol = 1;
					NewMemSymbol->SymbolQty = 1;

					NewMemSymbol->Next = MemCell->Symbols;
					MemCell->Symbols = NewMemSymbol;
				}
				else {
					MemCell->Symbols->Symbol = Character;
					MemCell->Symbols->CurrSymbol = 1;
					MemCell->Symbols->SymbolQty = 1;
				}
			}
		}
    }
	return MemCell;
}

void MoveMemHead(char Direction) {
    if (Direction == 'L') {
        if (CurrMemPosition->Left != NULL) {
			// Dummy cell
			if (CurrMemPosition->Left->Symbols->SymbolQty == 0) {
				CurrMemPosition = CurrMemPosition->Left;
				MoveMemHead('L');
				return;
			}

			// Not dummy
			if (CurrMemPosition->Left->Symbols->BranchID <= CurrBranchID && CurrMemPosition->Symbols->CurrSymbol <= 1) {
				CurrMemPosition = CurrMemPosition->Left;
			} else if (CurrMemPosition->Left->Symbols->BranchID > CurrBranchID && CurrMemPosition->Symbols->CurrSymbol <= 1) {
				CurrMemPosition = CurrMemPosition->Left;
				MoveMemHead('L');
			} else if (CurrMemPosition->Symbols->CurrSymbol > 1) {
				CurrMemPosition->Symbols->CurrSymbol--;
			}
        } else {
			if (CurrMemPosition->Symbols->CurrSymbol > 1) {
				CurrMemPosition->Symbols->CurrSymbol--;
			} else {
				CurrMemPosition->Left = malloc(sizeof(Cell));
				CurrMemPosition->Left->Symbols = NULL;
				CurrMemPosition->Left->Left = NULL;
				CurrMemPosition->Left->Right = CurrMemPosition;
				CurrMemPosition = WriteOnTape(CurrMemPosition->Left, '_');

			}
        }
    } else if (Direction == 'R') {
        if (CurrMemPosition->Right != NULL) {
			// Dummy cell
			if (CurrMemPosition->Right->Symbols->SymbolQty == 0) {
				CurrMemPosition = CurrMemPosition->Right;
				MoveMemHead('R');
				return;
			}

			if (CurrMemPosition->Right->Symbols->BranchID <= CurrBranchID && CurrMemPosition->Symbols->CurrSymbol >= CurrMemPosition->Symbols->SymbolQty) {
				CurrMemPosition = CurrMemPosition->Right;
			} else if (CurrMemPosition->Right->Symbols->BranchID > CurrBranchID && CurrMemPosition->Symbols->CurrSymbol >= CurrMemPosition->Symbols->SymbolQty) {
				CurrMemPosition = CurrMemPosition->Right;
				MoveMemHead('R');
			} else if (CurrMemPosition->Symbols->CurrSymbol < CurrMemPosition->Symbols->SymbolQty) {
				CurrMemPosition->Symbols->CurrSymbol++;
			}
        } else {
			if (CurrMemPosition->Symbols->CurrSymbol < CurrMemPosition->Symbols->SymbolQty) {
				CurrMemPosition->Symbols->CurrSymbol++;
			} else {
				CurrMemPosition->Right = malloc(sizeof(Cell));
				CurrMemPosition->Right->Symbols = NULL;
				CurrMemPosition->Right->Right = NULL;
				CurrMemPosition->Right->Left = CurrMemPosition;
				CurrMemPosition = WriteOnTape(CurrMemPosition->Right, '_');
				
			}
        }
    } else if (Direction == 'S') {

    } else {
        printf("Invalid direction!\n");
    }
}

void StackPush(Transition * Trans) {
    StackElem * NewElem = malloc(sizeof(StackElem));
    NewElem->Next = Stack;
    NewElem->MemPositionBuffer = CurrMemPosition;
	NewElem->CurrSymbolBuffer = CurrMemPosition->Symbols->CurrSymbol;
    NewElem->BranchID = CurrBranchID;
    NewElem->MovesBuffer = Moves;
    NewElem->Trans = Trans;

    Stack = NewElem;
}

StackElem * StackPop() {
    if (Stack != NULL) {
        StackElem * PoppedElem = Stack;
        Stack = Stack->Next;
        return PoppedElem;
    } else return NULL;
}

void RunInputs() {
	// Debugging TM setting phase
    /*TreeNode * DebugTree = TM->root;
    InOrderTreeWalk(DebugTree);
    printf("Max moves = %d\n", Moves);*/

    int Result;
    unsigned int MovesTmp = Moves;

    while (InitTape() != 0) {
        InitStack();

        Result = RunTM();

        if (Result == 1 || Result == 0) {
            printf("%d\n", Result);
        } else if (Result == 2) {
            printf("%c\n", 'U');
        }

        FreeStack();

        CurrBranchID = 0;
        Moves = MovesTmp;
		FlushMemorySymbols(-1);

        MemoryTape->Right = NULL;
        MemoryTape->Left = NULL;
        MemoryTape->Symbols = NULL;
		MemoryTape = WriteOnTape(MemoryTape, '_');
    }
}

void InitStack() {
    char FirstSymbol = MemoryTape->Symbols->Symbol;
    TransitionList * TransTmp = SearchReadCharTransList(SearchNode(TM, TM->root, 0)->StatePtr, FirstSymbol);

	if (TransTmp != NULL)
	{
		Transition * CurrTransTmp = TransTmp->Transitions;

		CurrBranchID++;

		while (CurrTransTmp != NULL)
		{
			StackPush(CurrTransTmp);
			CurrTransTmp = CurrTransTmp->NextTransition;
		}

		if (TransTmp->Transitions->NextTransition == NULL)
		{
			CurrBranchID--;
		}
	}    
}

// Returns 0 if there are no input left
int InitTape() {
    int InputSymbol;
    Cell * MemoryTapeTmp = MemoryTape;

    InputSymbol = getchar();
    if (InputSymbol == EOF) {
        return 0;
    }

    do {
        MemoryTapeTmp = WriteOnTape(MemoryTapeTmp, (char) InputSymbol);

        if (MemoryTapeTmp->Right != NULL) {
            MemoryTapeTmp = MemoryTapeTmp->Right;
        } else {
            MemoryTapeTmp->Right = malloc(sizeof(Cell));
            MemoryTapeTmp->Right->Symbols = NULL;
			MemoryTapeTmp->Right->Right = NULL;
			MemoryTapeTmp->Right->Left = MemoryTapeTmp;
			MemoryTapeTmp = WriteOnTape(MemoryTapeTmp->Right, '_');

        }
    } while ((InputSymbol = getchar()) != '\n' && InputSymbol != EOF);

	CurrMemPosition = MemoryTapeTmp->Left;
	while (CurrMemPosition != MemoryTape || CurrMemPosition->Symbols->CurrSymbol > 1) {
		MoveMemHead('L');
	}

    return 1;
};

/*int RunTM(State * CurrentState) {     // Recursive function for RunTM. Delete if not necessary
    if(Moves <= 0) {
        return 2;
    }

    if (CurrentState->IsAcceptanceState == true) {
        return 1;
    }

    char ReadMem = CurrMemPosition->Symbol;
    TransitionList * TransList = SearchReadCharTransList(CurrentState, ReadMem);

    if (TransList == NULL) {
        return 0;
    }

    Transition * CurrentTransition = TransList->Transitions;
    Cell * CurrTmp = CurrMemPosition;
    char SymbolTmp = CurrMemPosition->Symbol;
	int ExecResult = 0;

    while (CurrentTransition != NULL) {
		// Execute transition
        CurrMemPosition->Symbol = CurrentTransition->Write;
        MoveMemHead(CurrentTransition->HeadMoveDirection);
        Moves--;

        ExecResult = RunTM(CurrentTransition->ToState);
		
		Moves++;
		CurrMemPosition = CurrTmp;
		CurrMemPosition->Symbol = SymbolTmp;

        if (ExecResult == 1) {
            return ExecResult;
		}

        CurrentTransition = CurrentTransition->NextTransition;
    }

    Moves++;
    CurrMemPosition = CurrTmp;
    CurrMemPosition->Symbol = SymbolTmp;
    return ExecResult;
}*/

int RunTM() {       // Iterative version of RunTM
    State * CurrentState = SearchNode(TM, TM->root, 0)->StatePtr;
    StackElem * CurrStack;
    char Input;
    int AreMovesOver = 0;
	
	CurrStack = StackPop();

	if (CurrStack == NULL)
	{
		return 0;
	}

    do {
        if (CurrStack->BranchID > CurrBranchID) {
            Transition * CurrTransition = CurrStack->Trans;

            // Exec transition
			CurrMemPosition = WriteOnTape(CurrMemPosition, CurrTransition->Write);
            MoveMemHead(CurrTransition->HeadMoveDirection);
            CurrentState = CurrTransition->ToState;
            Moves--;

        } else if (CurrStack->BranchID <= CurrBranchID){
            Transition * CurrTransition = CurrStack->Trans;

            FlushMemorySymbols(CurrStack->BranchID - 1);
            CurrMemPosition = CurrStack->MemPositionBuffer;
			CompressionFixup(CurrMemPosition);
			CurrMemPosition->Symbols->CurrSymbol = CurrStack->CurrSymbolBuffer;
			Moves = CurrStack->MovesBuffer;
			CurrBranchID = CurrStack->BranchID;

            // Exec transition
			CurrMemPosition = WriteOnTape(CurrMemPosition, CurrTransition->Write);
            MoveMemHead(CurrTransition->HeadMoveDirection);
            CurrentState = CurrTransition->ToState;
            Moves--;

		}

		// Update stack with new transitions
		Input = CurrMemPosition->Symbols->Symbol;
		TransitionList * TransList = SearchReadCharTransList(CurrentState, Input);

		if (TransList != NULL && Moves > 0)
		{
			Transition * TransitionTemp = TransList->Transitions;

			CurrBranchID++;
			int AddedTrans = 0;

			while (TransitionTemp != NULL)
			{
				if (!(TransitionTemp->HeadMoveDirection == 'S' && TransitionTemp->Read == TransitionTemp->Write && TransitionTemp->ToState == CurrentState)) {
					StackPush(TransitionTemp);
					AddedTrans++;
					TransitionTemp = TransitionTemp->NextTransition;
				}
				else {
					AreMovesOver = 2;
				}
				
			}

			if (AddedTrans <= 1) {
				CurrBranchID--;
			}
		}

		if (Moves <= 0) {			
			AreMovesOver = 2;
		} else if (CurrentState->IsAcceptanceState == true)	{
			free(CurrStack);
			return 1;
		}

        free(CurrStack);
		CurrStack = StackPop();		
    } while (CurrStack != NULL);

    return AreMovesOver;
}

// BranchID = -1 if complete symbols
void FlushMemorySymbols(int BranchID) {
    // Flush right side
    FreeRightMemory(BranchID);

    // Flush left side
    FreeLeftMemory(BranchID);

	// Flush center
	if (MemoryTape->Symbols != NULL && MemoryTape->Symbols->BranchID > BranchID)
	{
		Symbol *CurrSymbol = MemoryTape->Symbols;

		while (CurrSymbol != NULL && CurrSymbol->BranchID > BranchID)
		{
			Symbol *SymbolTmp = CurrSymbol;
			CurrSymbol = CurrSymbol->Next;

			free(SymbolTmp);
		}

		MemoryTape->Symbols = CurrSymbol;
	}
}

void FreeMemory() {
    FlushMemorySymbols(-1);
    free(MemoryTape);
}

void FreeRightMemory(int BranchID) {
    // Flush right side
    // Positioning temp head to rightmost position
    Cell * MemoryTapeTmp = MemoryTape;
    while (MemoryTapeTmp->Right != NULL) {
        MemoryTapeTmp = MemoryTapeTmp->Right;
    }

    while (MemoryTapeTmp != MemoryTape) {
        Cell * LeftMemTmp = MemoryTapeTmp->Left;

        if (MemoryTapeTmp->Symbols != NULL && MemoryTapeTmp->Symbols->BranchID > BranchID) {
            Symbol * CurrSymbol = MemoryTapeTmp->Symbols;

            while (CurrSymbol != NULL && CurrSymbol->BranchID > BranchID) {
                Symbol * SymbolTmp = CurrSymbol;
                CurrSymbol = CurrSymbol->Next;

                free(SymbolTmp);
            }

            MemoryTapeTmp->Symbols = CurrSymbol;
            if (MemoryTapeTmp->Symbols == NULL) {
                MemoryTapeTmp->Left->Right = MemoryTapeTmp->Right;

				if (MemoryTapeTmp->Right != NULL) {
					MemoryTapeTmp->Right->Left = MemoryTapeTmp->Left;
				}

                free(MemoryTapeTmp);
            }
        }
        MemoryTapeTmp = LeftMemTmp;
    }
}

void FreeLeftMemory(int BranchID) {
    // Flush left side
    // Moving temp head to leftmost position
    Cell * MemoryTapeTmp = MemoryTape;
    while (MemoryTapeTmp->Left != NULL) {
        MemoryTapeTmp = MemoryTapeTmp->Left;
    }

    while (MemoryTapeTmp != MemoryTape) {
		Cell * RightMemTmp = MemoryTapeTmp->Right;

        if (MemoryTapeTmp->Symbols != NULL && MemoryTapeTmp->Symbols->BranchID > BranchID) {
            Symbol * CurrSymbol = MemoryTapeTmp->Symbols;

            while (CurrSymbol != NULL && CurrSymbol->BranchID > BranchID) {
                Symbol * SymbolTmp = CurrSymbol;
                CurrSymbol = CurrSymbol->Next;

                free(SymbolTmp);
            }

			MemoryTapeTmp->Symbols = CurrSymbol;
			if (MemoryTapeTmp->Symbols == NULL) {
				MemoryTapeTmp->Right->Left = MemoryTapeTmp->Left;

				if (MemoryTapeTmp->Left != NULL) {
					MemoryTapeTmp->Left->Right = MemoryTapeTmp->Right;
				}

				free(MemoryTapeTmp);
			}
        }

		MemoryTapeTmp = RightMemTmp;
    }
}

// Fixes CurrSymbol variable for left and right half of memory
void CompressionFixup(Cell * MemCell) {
	Cell * MemCellTmp = MemCell;

	while (MemCellTmp->Right != NULL) {
		MemCellTmp = MemCellTmp->Right;
		MemCellTmp->Symbols->CurrSymbol = 1;
	}

	MemCellTmp = MemCell;

	while (MemCellTmp->Left != NULL) {
		MemCellTmp = MemCellTmp->Left;
		MemCellTmp->Symbols->CurrSymbol = MemCellTmp->Symbols->SymbolQty;
	}

}

void FreeStack() {
    StackElem * StackTmp = Stack;

    while (StackTmp != NULL) {
        StackTmp = StackPop();
        free(StackTmp);
    }
}

void FreeTM() {
    FreeTransitions(TM->root);
    FreeNodes(TM->root);

    free(TM->nil);
    free(TM);
}

void FreeTransitions(TreeNode * x) {
    // For every tree node (state)
    if (x != TM->nil) {
        // Free left tree
        FreeTransitions(x->left);

        TransitionList * Character = x->StatePtr->CharacterList;
        // For every Character struct. in character list
        while (Character != NULL) {
            Transition * Transitions = Character->Transitions;

            // For every transition in list
            while (Transitions != NULL) {
                Transition * TempTrans = Transitions;
                Transitions = Transitions->NextTransition;
                // Free transition and repeat with next
                free(TempTrans);
            }

            // Transition list is now NULL
            Character->Transitions = NULL;

            TransitionList * Temp = Character;
            Character = Character->Next;
            // Free Character and repeat with next in list
            free(Temp);
        }

        // Character list is now NULL
        x->StatePtr->CharacterList = NULL;

        // Free right tree
        FreeTransitions(x->right);
    }
}

void FreeNodes(TreeNode * x) {
    // For every tree node (state)
    if (x != TM->nil) {
        // Create temporary right tree ptr
        TreeNode * RightTree = x->right;

        // Free left tree
        FreeNodes(x->left);

        // Free this node
        free(x->StatePtr);
        free(x);

        // Free right tree through temp ptr
        FreeNodes(RightTree);
    }
}