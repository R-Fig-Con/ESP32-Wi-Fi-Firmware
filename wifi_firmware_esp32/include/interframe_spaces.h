#define SIFS 2100 // Per most current measurement, should be at least 2000

#define BACKOFF_TIME_SLOT 1182 // following proprtion of ieee 802.11a between sifs and backoff slot

#define DIFS SIFS + (2 * BACKOFF_TIME_SLOT) // from wikipedia definition
#define EIFS SIFS + DIFS                    // wiki definition: Transmission time of Ack frame at lowest phy mandatory rate + SIFS + DIFS
