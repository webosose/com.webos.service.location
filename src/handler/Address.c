/*
 * Address.c
 *
 *  Created on: Feb 22, 2013
 *      Author: Abhishek
 */
#include <stdlib.h>
#include <string.h>
#include <Address.h>

Address *address_create(const gchar *street, const gchar *locality, const gchar *area, const gchar *region, const gchar *country,
                        const gchar *country_code, const gchar *postalcode)
{

    Address* addr = g_slice_new0(Address);
    if (addr == NULL) return NULL;
    addr->street = g_strdup(street);
    addr->locality = g_strdup(locality);
    addr->area = g_strdup(area);
    addr->region = g_strdup(region);
    addr->country = g_strdup(country);
    addr->countrycode = g_strdup(country_code);
    addr->postcode = g_strdup(postalcode);
    return addr;
}

void address_free(Address* addr)
{
    g_return_if_fail(addr);
    g_slice_free(Address, addr);
}

