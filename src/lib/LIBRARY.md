## Using the ledmon LED library
   A library is built when enabled using `--enable-library` during `./configure` and can be used to add LED control to your existing applications.  Assuming the library
   is installed, follow the following steps.

1. Make edits to your application, this simple source example shows the key things needed to use the library.


```c
#include <stdlib.h>
#include <stdio.h>

// Include the library header
#include <led/libled.h>


// Use the slot getter functions to retrieve information about a slot
static inline void print_slot(struct led_slot_list_entry *s)
{
    printf("cntrl type: %d: slot: %-15s led state: %-15d device: %-15s\n",
        led_slot_cntrl(s), led_slot_id(s), led_slot_state(s), led_slot_device(s));
}

int main() {
    struct led_slot_list_entry *slot = NULL;
    struct led_ctx *ctx = NULL;
    struct led_slot_list *slot_list = NULL;
    led_status_t status;

    // Create a context
    status = led_new(&ctx);
    if (LED_STATUS_SUCCESS != status){
        printf("Failed to initialize library\n");
        return 1;
    }

    // Run a scan to find all supported hardware
    status = led_scan(ctx);
    if (LED_STATUS_SUCCESS != status){
        printf("led_scan failed %d\n", status);
        return 1;
    }

    // Retrieve slots that support LED management
    status = led_slots_get(ctx, &slot_list);
    if (LED_STATUS_SUCCESS != status) {
        printf("led_slots_get failed %d\n", status);
        return 1;
    }

    printf("Printing slots in order returned\n");
    while ((slot = led_slot_next(slot_list))) {
        print_slot(slot);
    }

    // Free the slot list
    led_slot_list_free(slot_list);

	// Free the library context
    led_free(ctx);
    return 0;
}
```

2. Compile and link

```bash
$ gcc -Wall `pkg-config --cflags --libs ledmon` list_slots.c -o list_slots
```

3. Run

```bash
$ ./list_slots
Printing slots in order returned
cntrl type: 3: slot: /dev/sg43-0     led state: 2               device: /dev/sdw
cntrl type: 3: slot: /dev/sg43-1     led state: 2               device: /dev/sdx
cntrl type: 3: slot: /dev/sg43-2     led state: 2               device: /dev/sdy
cntrl type: 3: slot: /dev/sg43-3     led state: 2               device: /dev/sdz
cntrl type: 3: slot: /dev/sg43-4     led state: 2               device: /dev/sdaa
cntrl type: 3: slot: /dev/sg43-5     led state: 2               device: /dev/sdab
cntrl type: 3: slot: /dev/sg43-6     led state: 2               device: /dev/sdac
cntrl type: 3: slot: /dev/sg43-7     led state: 2               device: /dev/sdad
cntrl type: 3: slot: /dev/sg43-8     led state: 2               device: /dev/sdae
cntrl type: 3: slot: /dev/sg43-9     led state: 2               device: /dev/sdaf
cntrl type: 3: slot: /dev/sg43-10    led state: 2               device: /dev/sdag
cntrl type: 3: slot: /dev/sg43-11    led state: 2               device: /dev/sdah
cntrl type: 3: slot: /dev/sg43-12    led state: 2               device: /dev/sdai
cntrl type: 3: slot: /dev/sg43-13    led state: 2               device: /dev/sdaj
cntrl type: 3: slot: /dev/sg43-14    led state: 2               device: /dev/sdak
cntrl type: 3: slot: /dev/sg43-15    led state: 2               device: /dev/sdal
$
```

4. For more information on API, please review the [led/libled.h](include/led/libled.h) header file.
