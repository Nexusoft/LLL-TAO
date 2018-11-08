#include <vector>
#include <stdint.h>

void TestCompress(std::vector<uint8_t>& vData, uint16_t nSize = 32)
{
    /* Loop until key is of desired size. */
    while(vData.size() > nSize)
    {
        /* Loop half of the key to XOR elements. */
        uint32_t nTotal = 0;
        for(int i = 0; (i * 8) < nSize && i < vData.size() / 8; i += 8)
        {
            if(i + 8 < vData.size())
            {
                *((uint64_t*)&vData[i]) ^= *((uint64_t*)&vData[vData.size() - ((i - 1) * 8)]);

                nTotal += 8;
            }
        }

        /* Resize the container to half its size. */
        vData.resize(vData.size() - nTotal);
    }
}

int main(int argc, char** argv)
{

}
