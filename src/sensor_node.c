/*
 * Lab IoT-style sensor node (simulation)
 * - Fake ADC readings (temperature, humidity, bus voltage)
 * - Periodic sampling loop
 * - CSV append to local storage path
 * - Optional Mosquitto publish (compile with -DUSE_MQTT=1 -lmosquitto)
 *
 * This is an educational reconstruction of a hardware-internship lab demo.
 * It does not talk to a real MCU/ADC.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#ifndef USE_MQTT
#define USE_MQTT 0
#endif

#if USE_MQTT
#include <mosquitto.h>
#endif

#define DEVICE_ID "LAB-NODE-01"
#define DEFAULT_CSV "./data/sensor_samples.csv"
#define DEFAULT_LOG "./logs/node.log"

static double fake_sensor(double base, double amp) {
    double n = ((double)rand() / (double)RAND_MAX) * 2.0 - 1.0;
    return base + amp * n;
}

static void ensure_parent_dirs(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *slash = strrchr(tmp, '/');
    if (!slash) return;
    *slash = '\0';
    char cmd[640];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", tmp);
    system(cmd);
}

static int append_csv(const char *path, const char *ts, double t, double h, double v) {
    ensure_parent_dirs(path);
    int need_header = 0;
    FILE *chk = fopen(path, "r");
    if (!chk) need_header = 1;
    else fclose(chk);

    FILE *f = fopen(path, "a");
    if (!f) return -1;
    if (need_header) {
        fprintf(f, "timestamp,device_id,temp_c,humidity_pct,bus_voltage_v\n");
    }
    fprintf(f, "%s,%s,%.2f,%.2f,%.3f\n", ts, DEVICE_ID, t, h, v);
    fclose(f);
    return 0;
}

static void iso_now(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(buf, n, "%Y-%m-%dT%H:%M:%S", &tm);
}

int main(int argc, char **argv) {
    int samples = 5;
    int daemon_mode = 0;
    int interval_ms = 2000;
    int mqtt = 0;
    const char *csv = DEFAULT_CSV;

    static struct option opts[] = {
        {"samples", required_argument, 0, 'n'},
        {"daemon", no_argument, 0, 'd'},
        {"interval-ms", required_argument, 0, 'i'},
        {"mqtt", no_argument, 0, 'm'},
        {"csv", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "n:di:mc:", opts, NULL)) != -1) {
        switch (c) {
            case 'n': samples = atoi(optarg); break;
            case 'd': daemon_mode = 1; break;
            case 'i': interval_ms = atoi(optarg); break;
            case 'm': mqtt = 1; break;
            case 'c': csv = optarg; break;
            default: break;
        }
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    printf("=== IoT Sensor Node Lab ===\n");
    printf("Device: %s\nStorage: %s\n", DEVICE_ID, csv);

#if USE_MQTT
    struct mosquitto *mosq = NULL;
    if (mqtt) {
        mosquitto_lib_init();
        mosq = mosquitto_new(DEVICE_ID, true, NULL);
        /* Lab broker defaults — override via env in real setups */
        const char *host = getenv("MQTT_HOST"); if (!host) host = "127.0.0.1";
        const char *user = getenv("MQTT_USER");
        const char *pass = getenv("MQTT_PASS");
        if (user && pass) mosquitto_username_pw_set(mosq, user, pass);
        if (mosquitto_connect(mosq, host, 1883, 30) != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "MQTT connect failed (is broker up?)\n");
        }
    }
#else
    if (mqtt) {
        fprintf(stderr, "Built without MQTT. Recompile with -DUSE_MQTT=1 -lmosquitto\n");
    }
#endif

    int i = 0;
    while (daemon_mode || i < samples) {
        char ts[64];
        iso_now(ts, sizeof(ts));
        double temp = fake_sensor(25.0, 2.0);
        double hum = fake_sensor(45.0, 5.0);
        double vbus = fake_sensor(3.30, 0.05);
        printf("[%02d] %s  temp=%.2f C  humidity=%.2f %%  Vbus=%.3f V\n",
               i + 1, ts, temp, hum, vbus);
        if (append_csv(csv, ts, temp, hum, vbus) != 0) {
            fprintf(stderr, "CSV write failed: %s\n", strerror(errno));
            return 1;
        }
#if USE_MQTT
        if (mqtt && mosq) {
            char payload[256];
            snprintf(payload, sizeof(payload),
                     "{\"device\":\"%s\",\"ts\":\"%s\",\"temp\":%.2f,\"humidity\":%.2f,\"vbus\":%.3f}",
                     DEVICE_ID, ts, temp, hum, vbus);
            mosquitto_publish(mosq, NULL, "lab/sensors/node01", (int)strlen(payload), payload, 0, false);
            mosquitto_loop(mosq, 0, 1);
        }
#endif
        i++;
        if (daemon_mode || i < samples) usleep((useconds_t)interval_ms * 1000);
        if (!daemon_mode && i >= samples) break;
    }

    printf("OK: samples written to %s\n", csv);
#if USE_MQTT
    if (mosq) { mosquitto_disconnect(mosq); mosquitto_destroy(mosq); mosquitto_lib_cleanup(); }
#endif
    return 0;
}
