// A realistic source file for the end-to-end test: everything here is inside the supported
// C++ subset, and everything the subset supports appears here at least once.

const int table_size = 0x100;
const int flag_mask = 0b1010;

const char* greeting = "hello, \"world\"\n";
char newline = '\n';

double scale_factor = 1.5;

int checksum(const char* data, int length);

/*
 * Sums the bytes of a buffer, wrapping at the table size.
 * The classic block comment spans several lines and contains * stars ** inside.
 */
int checksum(const char* data, int length)
{
    int total = 0;

    for (int index = 0; index < length; ++index)
    {
        total = (total + data[index]) % table_size; // wrap per step
    }

    return total;
}

bool is_flagged(int value)
{
    return (value & flag_mask) != 0;
}

int classify(int value)
{
    if (is_flagged(value))
    {
        return 1;
    }
    else if (value == 0)
    {
        return 0;
    }

    int result = -1;

    while (value > table_size)
    {
        value = value / 2; /* halve until it fits */
        result = result + 1;
    }

    do
    {
        --value;
    } while (value > 0);

    return result;
}

int main()
{
    const char* message = greeting;
    int total = checksum(message, 16);

    int* cursor = &total;
    *cursor = *cursor + 1;

    return classify(total) >= 0 ? 0 : 1;
}
