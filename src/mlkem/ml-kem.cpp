#include "ml-kem.h"

#include <pqc/ml-kem.h>

#include <buffer.h>
#include <mlkem/indcpa.h>
#include <mlkem/params.h>
#include <mlkem/symmetric.h>
#include <mlkem/verify.h>
#include <rng/random_generator.h>


template <size_t MODE> uint32_t MLKEMFactory<MODE>::cipher_id() const { return ParameterSets[MODE].CIPHER_ID; }

template <size_t MODE>
std::unique_ptr<PQC_Context> MLKEMFactory<MODE>::create_context_asymmetric(
    const ConstBufferView & public_key, const ConstBufferView & private_key
) const
{
    check_size_or_empty(private_key, ParameterSets[MODE].PRIVATE_KEY_LEN);
    check_size_or_empty(public_key, ParameterSets[MODE].PUBLIC_KEY_LEN);
    return std::make_unique<MLKEMContext<MODE>>(public_key, private_key);
}

template <size_t MODE> void MLKEMContext<MODE>::generate_keypair()
{
    auto [public_key_view, private_key_view] =
        allocate_keys(ParameterSets[MODE].PUBLIC_KEY_LEN, ParameterSets[MODE].PRIVATE_KEY_LEN);

    auto [sk_part, pk_part, pk_hash, z] = private_key_view.split(
        ParameterSets[MODE].POLYVEC_SIZE, ParameterSets[MODE].PUBLIC_KEY_LEN, ML_RH_SIZE, ML_RH_SIZE
    );

    indcpa_keypair(public_key_view, sk_part, MODE, &get_random_generator());
    pk_part.store(public_key_view);
    function_H(public_key_view, pk_hash);
    get_random_generator().random_bytes(z);
}

template <size_t MODE>
void MLKEMContext<MODE>::kem_encapsulate_secret(const BufferView & message, const BufferView & shared_secret)
{
    if (message.size() != ParameterSets[MODE].MESSAGE_LEN || shared_secret.size() != ParameterSets[MODE].SHARED_LEN)
    {
        throw BadLength();
    }

    StackBuffer<2 * ML_RH_SIZE> buf;
    auto [m, pk_hash] = buf.split(ML_RH_SIZE, ML_RH_SIZE);
    StackBuffer<2 * ML_RH_SIZE> kr;
    auto [K, r] = kr.split(ML_RH_SIZE, ML_RH_SIZE);

    get_random_generator().random_bytes(m);

    function_H(public_key(), pk_hash);
    function_G(buf, kr);

    indcpa_enc(message.data(), buf.const_data(), public_key(), r.const_data(), MODE);
    shared_secret.store(K);
}

template <size_t MODE> size_t MLKEMFactory<MODE>::get_length(uint32_t type) const
{
    switch (type)
    {
    case PQC_LENGTH_PUBLIC:
        return ParameterSets[MODE].PUBLIC_KEY_LEN;
    case PQC_LENGTH_PRIVATE:
        return ParameterSets[MODE].PRIVATE_KEY_LEN;
    case PQC_LENGTH_MESSAGE:
        return ParameterSets[MODE].MESSAGE_LEN;
    case PQC_LENGTH_SHARED:
        return ParameterSets[MODE].SHARED_LEN;
    }
    return 0;
}

template <size_t MODE>
void MLKEMContext<MODE>::kem_decapsulate_secret(ConstBufferView message, BufferView shared_secret) const
{
    if (message.size() != ParameterSets[MODE].MESSAGE_LEN || shared_secret.size() != ParameterSets[MODE].SHARED_LEN)
    {
        throw BadLength();
    }

    uint8_t buf[2 * ML_RH_SIZE];
    /* Will contain key, coins */
    uint8_t kr[2 * ML_RH_SIZE];
    uint8_t cmp[ParameterSets[MODE].MESSAGE_LEN];

    const uint8_t * sk = private_key().const_data();
    ConstBufferView pk = private_key().mid(ParameterSets[MODE].POLYVEC_SIZE, ParameterSets[MODE].PUBLIC_KEY_LEN);

    indcpa_dec(buf, message.const_data(), private_key(), MODE);

    /* Multitarget countermeasure for coins + contributory KEM */
    for (size_t i = 0; i < ML_RH_SIZE; ++i)
        buf[ML_RH_SIZE + i] = sk[ParameterSets[MODE].PRIVATE_KEY_LEN - 2 * ML_RH_SIZE + i];
    hash_g(kr, buf, 2 * ML_RH_SIZE);

    /* coins are in kr+ML_RH_SIZE */
    indcpa_enc(cmp, buf, pk, kr + ML_RH_SIZE, MODE);

    int fail = verify(message.const_data(), cmp, ParameterSets[MODE].MESSAGE_LEN);

    /* overwrite coins in kr with H(c) */
    hash_h(kr + ML_RH_SIZE, message.const_data(), ParameterSets[MODE].MESSAGE_LEN);

    /* Overwrite pre-k with z on re-encryption failure */
    cmov(kr, sk + ParameterSets[MODE].PRIVATE_KEY_LEN - ML_RH_SIZE, ML_RH_SIZE, static_cast<uint8_t>(fail));

    if (fail)
    {
        uint8_t z_ct[ML_RH_SIZE + ParameterSets[MODE].MESSAGE_LEN];
        BufferView x(z_ct, ML_RH_SIZE + ParameterSets[MODE].MESSAGE_LEN);
        x.mid(0, ML_RH_SIZE).store(ConstBufferView(kr, ML_RH_SIZE));
        x.mid(ML_RH_SIZE, ParameterSets[MODE].MESSAGE_LEN).store(message);
        function_J(x, shared_secret);
    }
    else
    {
        shared_secret.store(ConstBufferView(kr, ML_RH_SIZE));
    }
}

template <size_t MODE> size_t MLKEMContext<MODE>::get_length(uint32_t type) const
{
    return MLKEMFactory<MODE>().get_length(type);
}

template class MLKEMFactory<ML_KEM_512>;
template class MLKEMFactory<ML_KEM_768>;
template class MLKEMFactory<ML_KEM_1024>;
