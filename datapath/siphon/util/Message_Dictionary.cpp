//
// Created by Sijia Chen on 2020/5/5.
//

#include <cstdio>
#include <cstring>

#include "Message_Dictionary.h"


namespace siphon {
    namespace util {


        DictionaryRef DictionaryCreate( size_t size, DictionaryCallbacks * callbacks )
        {
            struct Dictionary      * dictionary_ref;
            struct DictionaryItem ** items_pointer;

            if( size == 0 )
            {
                return NULL;
            }

            dictionary_ref = static_cast<Dictionary *>(calloc(sizeof(struct Dictionary), 1));
            items_pointer = static_cast<DictionaryItem **>(calloc(sizeof(struct DictionaryItem *), size));

            if( dictionary_ref == NULL || items_pointer == NULL )
            {
                free( dictionary_ref );
                free( items_pointer );

                return NULL;
            }

            dictionary_ref->items = items_pointer;
            dictionary_ref->size  = size;

            if( callbacks )
            {
                dictionary_ref->callbacks = *( callbacks );
            }

            return dictionary_ref;
        }

        void DictionaryRelease( DictionaryRef dictionary_ref )
        {
            if( dictionary_ref == NULL )
            {
                return;
            }

            DictionaryClear( dictionary_ref );

            free( dictionary_ref->items );
            free( dictionary_ref );
        }

        void DictionaryClear( DictionaryRef dictionary_ref )
        {
            size_t                  i;
            struct DictionaryItem * item;
            struct DictionaryItem * del;

            if( dictionary_ref == NULL )
            {
                return;
            }

            if( dictionary_ref->items != NULL )
            {
                for( i = 0; i < dictionary_ref->size; i++ )
                {
                    item          = dictionary_ref->items[ i ];
                    dictionary_ref->items[ i ] = NULL;

                    while( item )
                    {
                        del  = item;
                        item = item->next;

                        if( dictionary_ref->callbacks.kRelease )
                        {
                            dictionary_ref->callbacks.kRelease( del->key );
                        }

                        if( dictionary_ref->callbacks.vRelease )
                        {
                            dictionary_ref->callbacks.vRelease( del->value );
                        }

                        free( del );

                        dictionary_ref->count--;
                    }
                }
            }
        }

        void DictionaryInsert( DictionaryRef dictionary_ref, const void * key, const void * value )
        {
            struct DictionaryItem * item;
            DictionaryHashCode      hash_code;

            if( dictionary_ref == NULL || dictionary_ref->items == NULL || key == NULL || value == NULL )
            {
                return;
            }

            item = DictionaryGetItem( dictionary_ref, key );

            if( item )
            {
                if( dictionary_ref->callbacks.vRelease )
                {
                    dictionary_ref->callbacks.vRelease( item->value );
                }

                if( dictionary_ref->callbacks.vRetain )
                {
                    item->value = dictionary_ref->callbacks.vRetain( value );
                }
                else
                {
                    item->value = value;
                }

                return;
            }

            item = static_cast<DictionaryItem *>(calloc(sizeof(struct DictionaryItem), 1));

            if( item == NULL )
            {
                return;
            }

            if( dictionary_ref->callbacks.kRetain )
            {
                item->key = dictionary_ref->callbacks.kRetain( key );
            }
            else
            {
                item->key = key;
            }

            if( dictionary_ref->callbacks.vRetain )
            {
                item->value = dictionary_ref->callbacks.vRetain( value );
            }
            else
            {
                item->value = value;
            }

            if( dictionary_ref->callbacks.kHash )
            {
                hash_code = dictionary_ref->callbacks.kHash( key );
            }
            else
            {
                hash_code = ( size_t )key;
            }

            hash_code = hash_code % dictionary_ref->size;

            item->next    = dictionary_ref->items[ hash_code ];
            dictionary_ref->items[ hash_code ] = item;

            dictionary_ref->count++;

            if( dictionary_ref->count >= dictionary_ref->size * DICTIONARY_MAX_LOAD_FACTOR )
            {
                DictionaryResize( dictionary_ref, dictionary_ref->size * DICTIONARY_GROWTH_FACTOR );
            }
        }

        void DictionaryRemove( DictionaryRef dictionary_ref, const void * key )
        {
            DictionaryHashCode      hash_code;
            struct DictionaryItem * item;
            struct DictionaryItem * prev;

            if( dictionary_ref == NULL || dictionary_ref->items == NULL || key == NULL )
            {
                return;
            }

            if( dictionary_ref->callbacks.kHash )
            {
                hash_code = dictionary_ref->callbacks.kHash( key );
            }
            else
            {
                hash_code = ( size_t )key;
            }

            hash_code = hash_code % dictionary_ref->size;

            item = dictionary_ref->items[ hash_code ];
            prev = NULL;

            while( item )
            {
                if( dictionary_ref->callbacks.kCompare == NULL && key != item->key )
                {
                    prev = item;
                    item = item->next;

                    continue;
                }
                else if( key != item->key && dictionary_ref->callbacks.kCompare( key, item->key ) == false )
                {
                    prev = item;
                    item = item->next;

                    continue;
                }

                if( prev == NULL )
                {
                    dictionary_ref->items[ hash_code ] = item->next;
                }
                else
                {
                    prev->next = item->next;
                }

                if( dictionary_ref->callbacks.kRelease )
                {
                    dictionary_ref->callbacks.kRelease( item->key );
                }

                if( dictionary_ref->callbacks.vRelease )
                {
                    dictionary_ref->callbacks.vRelease( item->value );
                }

                free( item );

                dictionary_ref->count--;

                break;
            }
        }

        bool DictionaryKeyExists( DictionaryRef dictionary_ref, const void * key )
        {
            return DictionaryGetItem( dictionary_ref, key ) != NULL;
        }

        const void * DictionaryGetValue( DictionaryRef dictionary_ref, const void * key )
        {
            struct DictionaryItem * item;

            item = DictionaryGetItem( dictionary_ref, key );

            if( item )
            {
                return item->value;
            }

            return NULL;
        }



        void DictionaryPrint( DictionaryRef dictionary_ref )
        {
            struct DictionaryItem * item;
            printf("\n The items in the Dictionary: ");
            if( dictionary_ref->count ){
                for( int i = 0;i < dictionary_ref->size; i++ )
                {
                    item = dictionary_ref->items[i];
                    while( item ){
                        int* same_messages_received_number = (int *) item->value;
                        printf(" %s | value : %d;", (char*)item->key, *same_messages_received_number);
                        item = item->next;
                    }
                }
            }
            printf("\n");
        }

        void GetKeyWithMaximumValue(DictionaryRef dictionary_ref, void * obtained_key, int key_size)
        {
            struct DictionaryItem * item;
            int flag_value=0;
            if( dictionary_ref->count ){
                for( int i = 0;i < dictionary_ref->size; i++ )
                {
                    item = dictionary_ref->items[i];
                    while( item ){
                        int* same_messages_received_number = (int *) item->value;
                        if(*same_messages_received_number > flag_value)
                        {
                            memcpy(obtained_key, item->key, key_size);
                            flag_value = *same_messages_received_number;
                        }
                        item = item->next;
                    }
                }
            }
        }


        void DictionarySwap( DictionaryRef destination_dictionary_ref, DictionaryRef source_dictionary_ref )
        {
            struct Dictionary tmp;

            if( destination_dictionary_ref == NULL || source_dictionary_ref == NULL )
            {
                return;
            }

            tmp     = *( destination_dictionary_ref );
            *( destination_dictionary_ref ) = *( source_dictionary_ref );
            *( source_dictionary_ref ) = tmp;
        }

        void DictionaryResize( DictionaryRef dictionary_ref, size_t size )
        {
            size_t                  i;
            DictionaryRef           temp_dictionary_ref;
            struct DictionaryItem * item;
            DictionaryCallbacks     callbacks;

            if( dictionary_ref == NULL || dictionary_ref->items == NULL )
            {
                return;
            }

            temp_dictionary_ref = DictionaryCreate( size, &( dictionary_ref->callbacks ) );

            if( temp_dictionary_ref == NULL )
            {
                return;
            }

            callbacks             = temp_dictionary_ref->callbacks;
            temp_dictionary_ref->callbacks.kRetain = NULL;
            temp_dictionary_ref->callbacks.vRetain = NULL;

            for( i = 0; i < dictionary_ref->size; i++ )
            {
                item = dictionary_ref->items[ i ];

                while( item )
                {
                    DictionaryInsert( temp_dictionary_ref, item->key, item->value );

                    item = item->next;
                }
            }

            temp_dictionary_ref->callbacks         = callbacks;
            dictionary_ref->callbacks.kRelease = NULL;
            dictionary_ref->callbacks.vRelease = NULL;

            DictionarySwap( dictionary_ref, temp_dictionary_ref );
            DictionaryRelease( temp_dictionary_ref );
        }


        DictionaryCallbacks DictionaryStandardStringCallbacks( void )
        {
            DictionaryCallbacks c;

            memset( &c, 0, sizeof( DictionaryCallbacks ) );

            c.kHash    = DictionaryHashStringCallback;
            c.kCompare = DictionaryCompareStringCallback;
            c.kRetain  = ( const void * ( * )( const void * ) )strdup;
            c.vRetain  = ( const void * ( * )( const void * ) )strdup;
            c.kRelease = ( void ( * )( const void * ) )free;
            c.vRelease = ( void ( * )( const void * ) )free;
            c.kPrint   = ( void ( * )( FILE *, const void * ) )fprintf;
            c.vPrint   = ( void ( * )( FILE *, const void * ) )fprintf;

            return c;
        }

        DictionaryHashCode DictionaryHashStringCallback( const void * key )
        {
            DictionaryHashCode    hash_code;
            unsigned char         c;
            const unsigned char * cp;

            if( key == NULL )
            {
                return 0;
            }

            cp = static_cast<const unsigned char *>(key);
            hash_code  = 0;

            while( ( c = *( cp++ ) ) )
            {
                hash_code = hash_code * DICTIONARY_HASH_MULTIPLIER + c;
            }

            return hash_code;
        }

        bool DictionaryCompareStringCallback( const void * k1, const void * k2 )
        {
            if( k1 == NULL || k2 == NULL )
            {
                return false;
            }
            if( k1 == k2 )
            {
                return true;
            }
            return 0 == strcmp((const char *)k1, (const char *)k2);
        }

        static struct DictionaryItem * DictionaryGetItem( DictionaryRef dictionary_ref, const void * key )
        {
            DictionaryHashCode      hash_code;
            struct DictionaryItem * item;

            if( dictionary_ref == NULL || dictionary_ref->items == NULL || key == NULL )
            {
                return NULL;
            }

            if( dictionary_ref->callbacks.kHash )
            {
                hash_code = dictionary_ref->callbacks.kHash( key );
            }
            else
            {
                hash_code = ( size_t )key;
            }

            hash_code    = hash_code % dictionary_ref->size;
            item = dictionary_ref->items[ hash_code ];

            while( item )
            {
                if( key == item->key )
                {
                    return item;
                }

                if( dictionary_ref->callbacks.kCompare && dictionary_ref->callbacks.kCompare( key, item->key ) )
                {
                    return item;
                }

                item = item->next;
            }

            return NULL;
        }


    }
}