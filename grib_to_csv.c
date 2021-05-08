/**
 * Copyright (c) 2021, Mergen Imeev.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

enum
{
	n_lat = 161,
	n_lon = 181,
	n_gen = n_lat * n_lon,
};

const int val_land = -1000000000;
const int val_default = 1000000000;
const double lat_max = 80.0;
const double lat_min = 0.0;
const double lon_max = 0.0;
const double lon_min = -90.0;
const double lat_delta = 0.5;
const double lon_delta = -0.5;

bool
is_right(double lat, double lon)
{
	double delta = fabs(lat - (int)lat);
	if (delta != 0. && delta != 0.5)
		return false;
	delta = fabs(lon - (int)lon);
	if (delta != 0. && delta != 0.5)
		return false;
	return true;
}

int
get_delta(double lat0, double lon0, double lat, double lon)
{
	if (fabs(lat - lat0) == 0.5 || fabs(lat - lat0) == lat_max)
		return n_lon - (int)(fabs(lon - lon0) / 0.5);
	if (fabs(lat - lat0) != 0.)
		return -1;
	return (int)(fabs(lon - lon0) / 0.5);
}

bool
get_next(double *lat, double *lon)
{
	if (*lon < lon_max) {
		*lon -= lon_delta;
		return false;
	}
	if (*lat > lat_min) {
		*lat -= lat_delta;
		*lon = lon_min;
		return false;
	}
	*lat = lat_max;
	*lon = lon_min;
	return true;
}

void
put_value(int *buf, int value)
{
	static int i = 0;
	buf[i++] = value;
	if (i == n_gen)
		i = 0;
}

void
print_header(FILE *fout)
{
	double lat = lat_max;
	double lon = lon_min;
	for (int i = 0; i < n_gen; ++i) {
		fprintf(fout, ",%.1lf_%.1lf", lat, lon);
		get_next(&lat, &lon);
	}
	fprintf(fout, "\n");
}

void
print_data(int n, int *line, bool *ocean, FILE *fout)
{
	fprintf(fout, "%d,", n);
	for (int i = 0; i < n_gen - 1; ++i) {
		if (ocean[i])
			fprintf(fout, "%d,", line[i]);
		else
			fprintf(fout, ",");
	}
	fprintf(fout, "%d\n", line[n_gen - 1]);
}

int
get_index(double lat, double lon)
{
	return (int)((lat_max - lat) / lat_delta) * n_lon +
	       (int)((lon_min - lon) / lon_delta);
}

void
get_ocean(bool *ocean)
{
	char buf[160];
	double lat = 0.1;
	double lon = 0.1;
	FILE *fin = fopen("ocean.txt", "r");
	while (fgets(buf, sizeof(buf), fin) != NULL) {
		if (sscanf(buf, "%lf%lf", &lat, &lon) == 0)
			continue;
		if (!is_right(lat, lon))
			continue;
		ocean[get_index(lat, lon)] = true;
	}
	fclose(fin);
}

int
main(void)
{
	char buf[160];
	int *line = malloc(n_gen * sizeof(*line));
	bool *ocean = calloc(n_gen, sizeof(*ocean));
	double lat = 0.1;
	double lon = 0.1;
	double val;
	double lat0;
	double lon0;
	double val0;
	int n = 0;
	int j = 0;
	FILE *fin = fopen("data10.txt", "r");
	FILE *fdata = fopen("data10.csv", "w");
	while (fgets(buf, sizeof(buf), fin) != NULL) {
		if (sscanf(buf, "%lf%lf%lf", &lat, &lon, &val) != 0 &&
		    is_right(lat, lon))
			break;
	}
	lat0 = lat;
	lon0 = lon;
	get_ocean(ocean);
	line[get_index(lat, lon)] = val;
	val0 = val;
	print_header(fdata);
	while (fgets(buf, sizeof(buf), fin) != NULL) {
		if (sscanf(buf, "%lf%lf%lf", &lat, &lon, &val) == 0)
			continue;
		if (!is_right(lat, lon))
			continue;
		int delta = get_delta(lat0, lon0, lat, lon);
		if (delta == -1 || delta > n_gen) {
			fprintf(stderr, "wrong delta: %lf, %lf\n", lat, lon);
			return -1;
		}
		for (int i = 0; i < delta - 1; ++i) {
			if (get_next(&lat0, &lon0)) {
				if (lat != lat_max || lon != lon_min) {
					fprintf(stderr, "wrong next line");
					return -1;
				}
				print_data(n, line, ocean, fdata);
				++n;
			}
			double tmp_val =
				(val0*(delta - 1 - i) + val * (i + 1)) / delta;
			fprintf(stderr, "Warning! Missing data: %d, %lf, %lf\n",
				n, lat0, lon0);
			line[get_index(lat0, lon0)] = (int)tmp_val;
		}
		if (get_next(&lat0, &lon0)) {
			if (lat != lat_max || lon != lon_min) {
				fprintf(stderr, "wrong next line");
				return -1;
			}
			print_data(n, line, ocean, fdata);
			++n;
		}
		if (lat != lat0 || lon != lon0) {
			fprintf(stderr, "wrong next: %lf, %lf\n", lat, lon);
			return -1;
		}
		line[get_index(lat, lon)] = val;
		val0 = val;
	}
	if (lat != lat_min || lon != lon_max) {
		int delta = get_delta(lat0, lon0, lat_min, lon_max);
		if (delta == -1 || delta > n_gen) {
			fprintf(stderr, "wrong delta: %lf, %lf\n", lat, lon);
			return -1;
		}
		for (int i = 0; i < delta; ++i)
			line[get_index(lat, lon)] = val_default;
	}
	print_data(n, line, ocean, fdata);
	free(line);
	free(ocean);
	fclose(fin);
	fclose(fdata);
	return 0;
}
