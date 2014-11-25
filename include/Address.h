/********************************************************************
 * (c) Copyright 2012. All Rights Reserved
 * LG Electronics.
 *
 * Project Name : Location framework
 * Group   : PD-4
 * Security  : Confidential
 ********************************************************************/

/********************************************************************
 * @File
 * Filename  : ADDRESS.h
 * Purpose  : Provides Accuracy related variables utility functions
 * Platform  : RedRose
 * Author(s)  : abhishek Srivastava
 * Email ID.  : abhishek.srivastava@lge.com
 * Creation Date : 24-06-2013
 *
 * Modifications :
 * Sl No  Modified by  Date  Version Description
 *
 *
 ********************************************************************/

#ifndef ADDRESS_H_
#define ADDRESS_H_
#include <glib.h>
#include <glib-object.h>
#include <Position.h>
G_BEGIN_DECLS

Address *address_create(const gchar *street, const gchar *locality, const gchar *area, const gchar *region,
                        const gchar *country,
                        const gchar *country_code, const gchar *postalcode);
void address_free(Address *addr);
G_END_DECLS
#endif /* ADDRESS_H_ */
