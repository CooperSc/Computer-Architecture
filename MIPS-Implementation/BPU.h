#include <cstdint>
#include <math.h>
using namespace std;
class BPU
{
    // this is a simple branch predictor that
    // predicts every branch as taken.
    // Implement your own branch predictor here,
    // and call it from your processor.
    // If you implement it in a self-contained way,
    // you can enter the branch predictor contest and
    // win a prize!
    vector<int> BHT;
    uint32_t GHR = 0;

public:
    int size = 16;

    BPU()
    {
        for (int i = 0; i < ((int)(pow(2, size))); i++)
        {
            BHT.push_back(0);
        }
    }

    void update(uint32_t pc, bool taken)
    {
        // call this when you
        // figure out whether the branch is actually taken
        if (BHT[(GHR ^ pc) % ((int)(pow(2, size)))] > 1)
        {
            BHT[(GHR ^ pc) % ((int)(pow(2, size)))] = min(BHT[(GHR ^ pc) % ((int)(pow(2, size)))] - 1 + 2 * ((int)taken), 3);
        }
        else
        {
            BHT[(GHR ^ pc) % ((int)(pow(2, size)))] = max(BHT[(GHR ^ pc) % ((int)(pow(2, size)))] - 1 + 2 * ((int)taken), 0);
        }
        GHR = GHR >> 1;
        GHR = GHR + ((int)(pow(2, size - 1))) * ((int)taken);
    };

    bool predict(uint32_t pc)
    {
        // call this during instruction fetch
        return (BHT[(GHR ^ pc) % ((int)(pow(2, size)))] > 1); // always predicts taken
    }

    void print()
    {
        for (int i = 0; i < ((int)(pow(2, size))); i++)
        {
            cout << "BHT[" << i << "]: " << BHT[i] << endl;
        }
        cout << "GHR: " << GHR << endl;
    }
};
