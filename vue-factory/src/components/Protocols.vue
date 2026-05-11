<template>
    <div :style="{
            '--text': (msg?.payload?.theme?.text || '#FFFFFF'),
            '--rgb0': (msg?.payload?.theme?.rgb_hex || '#000000'),
            '--rgb1': (msg?.payload?.theme?.rgb_hex || '#000000') + '44',
            '--rgb2': (msg?.payload?.theme?.rgb_hex || '#000000') + '99',
            '--rgb3': (msg?.payload?.theme?.rgb_hex || '#000000') + '22'
        }">

        <div class="flex justify-center">
            <div class="w-1 rounded-l-sm rounded-br-sm bg-[var(--rgb1)]"></div>
            <div class="">
                <div class="bg-[var(--rgb1)] px-4 py-6 rounded-r-sm">
                    <h2 class="text-3xl font-bold text-[var(--text)]">Протоколи</h2>
                </div>
                <ul class="m-12 flex flex-col gap-3 list-none">
                    <li v-for="(protocol, index) in availableProtocols" :key="index">
                        <label class="flex items-center justify-between gap-4 cursor-pointer">

                        <!-- hidden checkbox -->
                        <input
                            type="checkbox"
                            class="sr-only peer"
                            v-model="selectedProtocols"
                            :value="protocol.id"
                            @change="handleProtocolToggle(protocol.id)" 
                        />

                        <!-- switch -->
                        <div
                            class="w-11 h-6 flex items-center bg-[var(--text)] rounded-md p-1
                                peer-checked:bg-[var(--rgb0)] transition-colors duration-300"
                        >
                            <div
                                class="w-4 h-4 bg-[var(--rgb0)] peer-checked:bg-[var(--text)] rounded-md shadow-md transform transition-transform duration-300"
                                :class="{ 'translate-x-5': selectedProtocols.includes(protocol.id) }"
                            ></div>
                        </div>

                        <!-- label -->
                        <span class="text-xl select-none">
                            {{ protocol.id }}
                        </span>

                        </label>
                    </li>
                </ul>
                <div class="bg-[var(--rgb1)] h-1px px-4 py-6 rounded-r-sm">
                </div>
            </div>
        </div>
    </div>
</template>

<script>
    export default {
        props: ["msg"],
        data() {
            return {
                availableProtocols: [],
                selectedProtocols: [], 
            }
        },
        watch: {
            msg: function (newMsg) {
                let pData = newMsg?.payload?.protocol;
                
                if (pData) {
                    if (pData.allAvailable) {
                        this.availableProtocols = pData.allAvailable;
                    }
                    if (pData.allSelected !== undefined) {
                        this.selectedProtocols = pData.allSelected;
                    }
                }
            }
        },
        methods: {
            handleProtocolToggle(selectedProtocol) {
                const isActive = this.selectedProtocols.includes(selectedProtocol);
                
                console.log(`Протокол ${selectedProtocol} тепер:`, isActive ? "УВІМКНЕНО" : "ВИМКНЕНО");
                
                this.send({
                    ui_key: "protocol",
                    payload: {
                        protocol: selectedProtocol,
                        isActive: isActive,
                        allSelected: this.selectedProtocols,
                        allAvailable: this.availableProtocols
                    }
                });
            }
        },
    }
</script>