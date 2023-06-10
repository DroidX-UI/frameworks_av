/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2020 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/
#ifndef BINDING_BLOB_TYPES_H
#define BINDING_BLOB_TYPES_H

#define BINDING_BLOB_SIZE 16

namespace dolby {

struct BBHighLow
{
    int64_t high;
    int64_t low;
};

union BindingBlobStorage
{
    unsigned char raw[BINDING_BLOB_SIZE];
    BBHighLow split;
};

} // namespace dolby

enum {
    DMS_INTF_PARAM_BINDING_BLOB_HIGH = 'BB_H',
    DMS_INTF_PARAM_BINDING_BLOB_LOW = 'BB_L'
};

#endif // BINDING_BLOB_TYPES_H
